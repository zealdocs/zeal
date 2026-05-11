/**
 * @id zeal/cpp/use-after-expired-lifetime
 * @name Use of object after its lifetime has ended (Qt-aware patch)
 * @description Accessing an object after its lifetime has ended can result in security vulnerabilities and undefined behavior. This is a local patched copy of the upstream `cpp/use-after-expired-lifetime` query that drops the over-broad "has a field which is a lifetime pointer/owner type" classification clauses, which misclassify any class with a private d-pointer (most Qt value types) as pointer-like.
 * @kind problem
 * @precision medium
 * @problem.severity error
 * @tags correctness
 *       security
 *       experimental
 *       external/cwe/cwe-416
 */

import cpp
import semmle.code.cpp.controlflow.Nullness

class StarOperator extends Operator {
  StarOperator() {
    this.hasName("operator*") and
    this.getNumberOfParameters() = 0
  }
}

class IncrementOperator extends Operator {
  IncrementOperator() {
    this.hasName("operator++") and
    this.getNumberOfParameters() = 0
  }
}

class StructureDerefOperator extends Operator {
  StructureDerefOperator() {
    this.hasName("operator->") and
    this.getNumberOfParameters() = 0
  }
}

class SubscriptOperator extends Operator {
  SubscriptOperator() {
    this.hasName("operator[]") and
    this.getNumberOfParameters() = 1
  }
}

/**
 * A type which is an `Indirection` type according to the Lifetime profile.
 *
 * An indirection type is either a `LifetimePointerType` or `LifetimeOwnerType`.
 */
abstract class LifetimeIndirectionType extends Type {
  /**
   * Gets the `DerefType` of this indirection type.
   *
   * This corresponds to the owned or pointed to type.
   */
  Type getDerefType() {
    result = this.(PointerType).getBaseType()
    or
    result = this.(ReferenceType).getBaseType()
    or
    exists(MemberFunction mf | mf.getDeclaringType() = this |
      result = mf.(StarOperator).getType().getUnspecifiedType().(ReferenceType).getBaseType()
      or
      result = mf.(SubscriptOperator).getType().getUnspecifiedType().(ReferenceType).getBaseType()
      or
      result =
        mf.(StructureDerefOperator).getType().getUnspecifiedType().(PointerType).getBaseType()
      or
      mf.getName() = "begin" and
      result = mf.getType().(LifetimePointerType).getDerefType()
    )
  }
}

/**
 * A lifetime owner type.
 *
 * A type which owns another object. For example, `std::unique_ptr`. Includes
 * `LifetimeSharedOwnerType`.
 *
 * NOTE (zeal patch): the upstream `Has a field which is a lifetime owner type`
 * clause is intentionally omitted — it caused widespread false positives on Qt
 * value classes that hold private d-pointers.
 */
class LifetimeOwnerType extends LifetimeIndirectionType {
  LifetimeOwnerType() {
    // Any shared owner types are also owner types
    this instanceof LifetimeSharedOwnerType
    or
    // This is a container type, or a type with a star operator and..
    (
      this instanceof ContainerType
      or
      exists(StarOperator mf | mf.getDeclaringType() = this)
    ) and
    // .. has a "user" provided destructor
    exists(Destructor d |
      d.getDeclaringType() = this and
      not d.isCompilerGenerated()
    )
    or
    // Any specified version of an owner type is also an owner type
    this.getUnspecifiedType() instanceof LifetimeOwnerType
    or
    // Derived from a public base class which is a owner type
    exists(ClassDerivation cd |
      cd = this.(Class).getADerivation() and
      cd.getBaseClass() instanceof LifetimeOwnerType and
      cd.getASpecifier().hasName("public")
    )
    or
    // Lifetime profile treats the following types as owner types, even though they don't fully
    // adhere to the requirements above
    this.(Class)
        .hasQualifiedName("std",
          ["stack", "queue", "priority_queue", "optional", "variant", "any", "regex"])
    or
    // Explicit annotation on the type
    this.getAnAttribute().getName().matches("gsl::Owner%")
  }
}

/**
 * A `ContainerType`, based on `[container.requirements]` with some adaptions to capture more real
 * world containers.
 */
class ContainerType extends Class {
  ContainerType() {
    // We use a simpler set of heuristics than the `[container.requirements]`, requiring only
    // `begin()`/`end()`/`size()` as the minimum API for something to be considered a "container"
    // type
    this.getAMemberFunction().getName() = "begin" and
    this.getAMemberFunction().getName() = "end" and
    this.getAMemberFunction().getName() = "size"
    or
    // This class is a `ContainerType` if it is constructed from a `ContainerType`. This is
    // important, because templates may not have instantiated all the required member functions
    exists(TemplateClass tc |
      this.isConstructedFrom(tc) and
      tc instanceof ContainerType
    )
  }
}

/**
 * A lifetime "shared owner" type.
 *
 * A shared owner is type that "owns" another object, and shares that ownership with other owners.
 * Examples include `std::shared_ptr` along with other reference counting types.
 *
 * NOTE (zeal patch): the upstream `Has a field which is a lifetime shared owner
 * type` clause is intentionally omitted. Qt's `QSharedDataPointer` /
 * `QExplicitlySharedDataPointer` themselves satisfy the shared-owner criteria
 * (operator*, user destructor, copy semantics); leaving the field clause in
 * would re-route the same Qt-value-class false positives through
 * `LifetimeSharedOwnerType` -> `LifetimeOwnerType`.
 */
class LifetimeSharedOwnerType extends Type {
  LifetimeSharedOwnerType() {
    /*
     * Find all types which can be dereferenced (i.e. have unary * operator), and are therefore
     * likely to be "owner"s or "pointer"s to other objects. We then consider these classes to be
     * shared owners if:
     *  - They can be copied (a unique "owner" type would not be copyable)
     *  - They can destroyed
     */

    // unary * (i.e. can be dereferenced)
    exists(StarOperator mf | mf.getDeclaringType() = this) and
    // "User" provided destructor
    exists(Destructor d |
      d.getDeclaringType() = this and
      not d.isCompilerGenerated()
    ) and
    // A copy constructor and copy assignment operator
    exists(CopyConstructor cc | cc.getDeclaringType() = this and not cc.isDeleted()) and
    exists(CopyAssignmentOperator cc | cc.getDeclaringType() = this and not cc.isDeleted())
    or
    // This class is a `SharedOwnerType` if it is constructed from a `SharedOwnerType`. This is
    // important, because templates may not have instantiated all the required member functions
    exists(TemplateClass tc |
      this.(Class).isConstructedFrom(tc) and
      tc instanceof LifetimeSharedOwnerType
    )
    or
    // Any specified version of a shared owner type is also a shared owner type
    this.getUnspecifiedType() instanceof LifetimeSharedOwnerType
    or
    // Derived from a public base class which is a shared owner type
    exists(ClassDerivation cd |
      cd = this.(Class).getADerivation() and
      cd.getBaseClass() instanceof LifetimeSharedOwnerType and
      cd.getASpecifier().hasName("public")
    )
    or
    // Lifetime profile treats the following types as shared owner types, even though they don't
    // fully adhere to the requirements above
    this.(Class).hasQualifiedName("std", "shared_future")
    or
    // Explicit annotation on the type
    this.getAnAttribute().getName().matches("gsl::SharedOwner%")
  }
}

/**
 * An `IteratorType`, based on `[iterator.requirements]` with some adaptions to capture more real
 * world iterators.
 */
class IteratorType extends Type {
  IteratorType() {
    // We consider anything with an increment and * operator to be sufficient to be an iterator type
    exists(StarOperator mf |
      mf.getDeclaringType() = this and mf.getType().getUnspecifiedType() instanceof ReferenceType
    ) and
    exists(IncrementOperator op |
      op.getDeclaringType() = this and op.getType().(ReferenceType).getBaseType() = this
    )
    or
    // Along with unspecified versions of the types above
    this.getUnspecifiedType() instanceof IteratorType
  }
}

/**
 * A lifetime pointer type.
 *
 * A type which points to another object. For example, `std::unique_ptr`. Includes
 * `LifetimeSharedOwnerType`.
 *
 * NOTE (zeal patch): the upstream `Has a field which is a lifetime pointer type`
 * clause is intentionally omitted — it caused widespread false positives on Qt
 * value classes that hold private d-pointers.
 */
class LifetimePointerType extends LifetimeIndirectionType {
  LifetimePointerType() {
    this instanceof IteratorType
    or
    this instanceof PointerType
    or
    this instanceof ReferenceType
    or
    // A shared owner type is a pointer type, but an owner type is not.
    this instanceof LifetimeSharedOwnerType and
    not this instanceof LifetimeOwnerType
    or
    this.(Class).hasQualifiedName("std", "reference_wrapper")
    or
    exists(Class vectorBool, UserType reference |
      vectorBool.hasQualifiedName("std", "vector") and
      vectorBool.getATemplateArgument() instanceof BoolType and
      reference.hasName("reference") and
      reference.getDeclaringType() = vectorBool and
      this = reference.getUnderlyingType()
    )
    or
    // Any specified version of a pointer type is also an owner type
    this.getUnspecifiedType() instanceof LifetimePointerType
    or
    // Derived from a public base class which is a pointer type
    exists(ClassDerivation cd |
      cd = this.(Class).getADerivation() and
      cd.getBaseClass() instanceof LifetimePointerType and
      cd.getASpecifier().hasName("public")
    )
    or
    // Explicit annotation on the type
    this.getAnAttribute().getName().matches("gsl::Pointer%")
  }
}

/** A full expression as defined in [intro.execution] of N3797. */
class FullExpr extends Expr {
  FullExpr() {
    // A full-expression is not a subexpression
    not this.getParent() instanceof Expr
    or
    // A sub-expression that is an unevaluated operand
    this.isUnevaluated()
  }
}

/** Gets the `FullExpression` scope of the `TemporaryObjectExpr`. */
FullExpr getTemporaryObjectExprScope(TemporaryObjectExpr toe) {
  result = toe.getUnconverted().getParent*()
}

/**
 * See `LifetimeLocalVariable` and subclasses.
 */
private newtype TLifetimeLocalVariable =
  TLocalScopeVariable(LocalScopeVariable lsv) { not lsv.isStatic() } or
  TTemporaryObject(TemporaryObjectExpr toe)

/**
 * A "LocalVariable" as defined by the lifetime profile.
 *
 * This includes newly introduced objects with a local scope.
 */
class LifetimeLocalVariable extends TLifetimeLocalVariable {
  string toString() { none() } // specified in sub-classes

  Type getType() { none() }
}

/**
 * A parameter or `LocalVariable`, used as a `LifetimeLocalVariable`
 */
class LifetimeLocalScopeVariable extends TLocalScopeVariable, LifetimeLocalVariable {
  LocalScopeVariable getVariable() { this = TLocalScopeVariable(result) }

  override Type getType() { result = this.getVariable().getType() }

  override string toString() { result = this.getVariable().toString() }
}

/**
 * A temporary object used as a `LifetimeLocalVariable`.
 */
class LifetimeTemporaryObject extends TTemporaryObject, LifetimeLocalVariable {
  TemporaryObjectExpr getTemporaryObjectExpr() { this = TTemporaryObject(result) }

  override Type getType() { result = this.getTemporaryObjectExpr().getType() }

  override string toString() { result = this.getTemporaryObjectExpr().toString() }
}

newtype TInvalidReason =
  /** LifetimeLocalVariable is invalid because it hasn't been initialized. */
  TUninitialized(DeclStmt ds, Variable v) { ds.getADeclaration() = v } or
  /** LifetimeLocalVariable is invalid because it points to a variable which has gone out of scope. */
  TVariableOutOfScope(LocalScopeVariable v, ControlFlowNode cfn) { goesOutOfScopeAt(v, cfn) } or
  /** LifetimeLocalVariable is invalid because it points to a temporary object expression which has gone out of scope. */
  TTemporaryOutOfScope(TemporaryObjectExpr toe) or
  /** LifetimeLocalVariable is invalid because it points to data held by an owner which has since been invalidated. */
  TOwnerModified(FunctionCall fc)

/**
 * A reason why a pointer may be invalid.
 */
class InvalidReason extends TInvalidReason {
  /** Holds if this reason indicates the pointer is accessed before the lifetime of an object began. */
  predicate isBeforeLifetime() { this instanceof TUninitialized }

  /** Holds if this reason indicates the pointer is accessed after the lifetime of an object has finished. */
  predicate isAfterLifetime() { not this.isBeforeLifetime() }

  /** Gets a description of the reason why this pointer may be invalid. */
  string getDescription() {
    exists(DeclStmt ds, Variable v |
      this = TUninitialized(ds, v) and
      result = "variable " + v.getName() + " was never initialized"
    )
    or
    exists(LocalScopeVariable v, ControlFlowNode cfn |
      this = TVariableOutOfScope(v, cfn) and
      result = "variable " + v.getName() + " went out of scope"
    )
    or
    exists(TemporaryObjectExpr toe |
      this = TTemporaryOutOfScope(toe) and
      result = "temporary object went out of scope"
    )
    or
    exists(FunctionCall fc |
      this = TOwnerModified(fc) and
      result = "owner type was modified"
    )
  }

  string toString() { result = this.getDescription() }

  /** Get an element that explains the reason for the invalid determination. */
  private Element getExplanatoryElement() {
    exists(DeclStmt ds |
      this = TUninitialized(ds, _) and
      result = ds
    )
    or
    exists(ControlFlowNode cfn |
      this = TVariableOutOfScope(_, cfn) and
      result = cfn
    )
    or
    exists(TemporaryObjectExpr toe |
      this = TTemporaryOutOfScope(toe) and
      result = getTemporaryObjectExprScope(toe)
    )
    or
    exists(FunctionCall fc |
      this = TOwnerModified(fc) and
      result = fc
    )
  }

  /**
   * Provides a `message` for use in alert messages.
   *
   * The message will contain a `$@` placeholder, for which `explanation` and `explanationDesc` are
   * the placeholder components which should be added as extra columns.
   */
  predicate hasMessage(string message, Element explanation, string explanationDesc) {
    message = "because the " + this.getDescription() + " $@." and
    explanation = this.getExplanatoryElement() and
    explanationDesc = "here"
  }
}

/**
 * A reason why a pointer may be null.
 */
newtype TNullReason =
  // Null because the `NullValue` was assigned
  TNullAssignment(NullValue e)

class NullReason extends TNullReason {
  /** Gets a description of the reason why this pointer may be null. */
  string getDescription() {
    exists(NullValue nv |
      this = TNullAssignment(nv) and
      result = "null value was assigned"
    )
  }

  string toString() { result = this.getDescription() }
}

/** See `PSetEntry` and subclasses. */
newtype TPSetEntry =
  /** Points to a lifetime local variable. */
  PSetVar(LifetimeLocalVariable lv) or
  /** Points to a lifetime local variable that represents an owner type. */
  PSetOwner(LifetimeLocalVariable lv, int level) {
    level = [0 .. 2] and lv.getType() instanceof LifetimeOwnerType
  } or
  /** Points to a global variable. */
  PSetGlobal() or
  /** A null pointer. */
  PSetNull(NullReason nr) or
  /** An invalid pointer, for the given reason. */
  PSetInvalid(InvalidReason ir) or
  /** An unknown pointer. */
  PSetUnknown()

/**
 * An entry in the points-to set for a particular "LifetimeLocalVariable" at a particular
 * point in the program.
 */
class PSetEntry extends TPSetEntry {
  string toString() {
    exists(LifetimeLocalVariable lv |
      this = PSetVar(lv) and
      result = "Var(" + lv.toString() + ")"
    )
    or
    this = PSetGlobal() and result = "global"
    or
    exists(LifetimeLocalVariable lv, int level |
      this = PSetOwner(lv, level) and
      result = "Owner(" + lv.toString() + "," + level + ")"
    )
    or
    exists(NullReason nr | this = PSetNull(nr) and result = "null because" + nr)
    or
    exists(InvalidReason ir | this = PSetInvalid(ir) and result = "invalid because " + ir)
    or
    this = PSetUnknown() and result = "unknown"
  }
}

/**
 * The "pmap" or "points-to map" for a "lifetime" local variable.
 */
predicate pointsToMap(ControlFlowNode cfn, LifetimeLocalVariable lv, PSetEntry ps) {
  if isPSetReassigned(cfn, lv)
  then ps = getAnAssignedPSetEntry(cfn, lv)
  else
    // Exclude unknown for now
    exists(ControlFlowNode pred, PSetEntry prevPSet |
      pred = cfn.getAPredecessor() and
      pointsToMap(pred, lv, prevPSet) and
      // Not PSetNull() and a non-null successor of a null check
      not exists(AnalysedExpr ae |
        ps = PSetNull(_) and
        cfn = ae.getNonNullSuccessor(lv.(LifetimeLocalScopeVariable).getVariable())
      ) and
      // lv is not out of scope at this node
      not goesOutOfScopeAt(lv.(LifetimeLocalScopeVariable).getVariable(), cfn)
    |
      // Propagate a PSetEntry from the predecessor node, so long as the
      // PSetEntry is not invalidated at this node
      ps = prevPSet and
      not exists(getAnInvalidation(prevPSet, cfn))
      or
      // Replace prevPSet with an invalidation reason at this node
      ps = getAnInvalidation(prevPSet, cfn)
    )
}

private predicate isPSetReassigned(ControlFlowNode cfn, LifetimeLocalVariable lv) {
  exists(DeclStmt ds |
    cfn = ds and
    ds.getADeclaration() = lv.(LifetimeLocalScopeVariable).getVariable() and
    lv.getType() instanceof PointerType
  )
  or
  exists(TemporaryObjectExpr toe |
    toe = lv.(LifetimeTemporaryObject).getTemporaryObjectExpr() and
    cfn = toe
  )
  or
  // Assigned a value
  cfn = lv.(LifetimeLocalScopeVariable).getVariable().getAnAssignedValue()
  or
  // If the address of a local var is passed to a function, then assume it initializes it
  exists(Call fc, AddressOfExpr aoe |
    cfn = aoe and
    fc.getAnArgument() = aoe and
    lv.(LifetimeLocalScopeVariable).getVariable() = aoe.getOperand().(VariableAccess).getTarget()
  )
}

/** Is the `lv` assigned or reassigned at this ControlFlowNode `cfn`. */
private PSetEntry getAnAssignedPSetEntry(ControlFlowNode cfn, LifetimeLocalVariable lv) {
  exists(DeclStmt ds |
    cfn = ds and
    ds.getADeclaration() = lv.(LifetimeLocalScopeVariable).getVariable()
  |
    lv.getType() instanceof PointerType and
    result = PSetInvalid(TUninitialized(ds, lv.(LifetimeLocalScopeVariable).getVariable()))
  )
  or
  exists(TemporaryObjectExpr toe |
    toe = lv.(LifetimeTemporaryObject).getTemporaryObjectExpr() and
    cfn = toe and
    result = PSetVar(lv)
  )
  or
  // Assigned a value
  exists(Expr assign |
    assign = lv.(LifetimeLocalScopeVariable).getVariable().getAnAssignedValue() and
    cfn = assign
  |
    if isKnownAssignmentType(assign)
    then knownAssignmentType(assign, result)
    else result = PSetUnknown()
  )
  or
  // If the address of a local var is passed to a function, then assume it initializes it
  exists(Call fc, AddressOfExpr aoe |
    cfn = aoe and
    fc.getAnArgument() = aoe and
    lv.(LifetimeLocalScopeVariable).getVariable() = aoe.getOperand().(VariableAccess).getTarget() and
    result = PSetUnknown()
  )
}

predicate isKnownAssignmentType(Expr assign) {
  assign = any(LocalScopeVariable lv).getAnAssignedValue() and
  (
    exists(Variable v | v = assign.(AddressOfExpr).getOperand().(VariableAccess).getTarget() |
      v instanceof LocalScopeVariable
      or
      v instanceof GlobalVariable
    )
    or
    // Assignment of a previous variable
    exists(VariableAccess va |
      va = assign and
      va.getTarget().(LocalScopeVariable).getType() instanceof LifetimePointerType
    )
    or
    assign instanceof NullValue
    or
    exists(FunctionCall fc |
      assign = fc and
      fc.getNumberOfArguments() = 0 and
      fc.getType() instanceof LifetimePointerType
    |
      // A function call is a product of its inputs (just handle qualifiers at the moment)
      exists(LifetimeLocalVariable lv |
        lv = TTemporaryObject(fc.getQualifier().getConversion())
        or
        lv = TLocalScopeVariable(fc.getQualifier().(VariableAccess).getTarget())
      |
        lv.getType() instanceof LifetimePointerType
        or
        lv.getType() instanceof LifetimeOwnerType
      )
    )
  )
}

/**
 * An expression which is assigned to a `LocalScopeVariable`, which has a known PSet value i.e. not
 * an "Unknown" PSet value.
 */
predicate knownAssignmentType(Expr assign, PSetEntry ps) {
  assign = any(LocalScopeVariable lv).getAnAssignedValue() and
  (
    // The assigned value is `&v`
    exists(Variable v | v = assign.(AddressOfExpr).getOperand().(VariableAccess).getTarget() |
      v instanceof LocalScopeVariable and
      (
        // If the variable we are taking the address of is a reference type, then we are really
        // taking the address of whatever the reference type "points-to". Use the `pointsToMap`
        // to determine viable `LifetimeLocalScopeVariable`s this could point to.
        if v.getType() instanceof ReferenceType
        then
          pointsToMap(assign.getAPredecessor(),
            any(LifetimeLocalScopeVariable lv | lv.getVariable() = v), ps)
        else
          // This assignment points-to `v` itself.
          ps = PSetVar(TLocalScopeVariable(v))
      )
      or
      // If the variable we are taking the address of is a reference variable, then this points-to
      // a global. If the variable we taking the address of is a reference type, we need to consider
      // that it might point-to a global, even if it is a LocalScopeVariable (this case is required
      // so that we still produce a result even if the pointsToMap is empty for `lv`).
      (v instanceof GlobalVariable or v.getType() instanceof ReferenceType) and
      ps = PSetGlobal()
    )
    or
    // Assignment of a previous variable
    exists(VariableAccess va |
      va = assign and
      va.getTarget().(LocalScopeVariable).getType() instanceof LifetimePointerType and
      // PSet of that become PSet of this
      pointsToMap(assign.getAPredecessor(),
        any(LifetimeLocalScopeVariable lv | lv.getVariable() = va.getTarget()), ps)
    )
    or
    // The `NullValue` class covers all types of null equivalent expressions. This case also handles
    // default and value initialization, where an "implicit" null value expression is added by the
    // extractor
    assign instanceof NullValue and ps = PSetNull(TNullAssignment(assign))
    or
    exists(FunctionCall fc |
      assign = fc and
      // If the assignment is being converted via a ReferenceDereferenceExpr, then
      // we are essentially copying the original object
      not assign.getFullyConverted() instanceof ReferenceDereferenceExpr and
      fc.getNumberOfArguments() = 0 and
      fc.getType() instanceof LifetimePointerType
    |
      // A function call is a product of its inputs (just handle qualifiers at the moment)
      exists(LifetimeLocalVariable lv |
        lv = TTemporaryObject(fc.getQualifier().getConversion())
        or
        lv = TLocalScopeVariable(fc.getQualifier().(VariableAccess).getTarget())
      |
        ps = PSetVar(lv) and lv.getType() instanceof LifetimePointerType
        or
        ps = PSetOwner(lv, 0) and lv.getType() instanceof LifetimeOwnerType
      )
    )
  )
}

/**
 * Holds if `cfn` is a node that occur directly after the local scope variable `lv` has gone out of scope.
 */
predicate goesOutOfScopeAt(LocalScopeVariable lv, ControlFlowNode cfn) {
  exists(BlockStmt scope |
    scope = lv.getParentScope() and
    scope.getAChild+() = cfn.getAPredecessor().getEnclosingStmt() and
    not scope.getAChild+() = cfn.getEnclosingStmt()
  )
}

PSetInvalid getAnInvalidation(PSetEntry ps, ControlFlowNode cfn) {
  exists(LifetimeLocalScopeVariable lv | ps = PSetVar(lv) |
    result = PSetInvalid(TVariableOutOfScope(lv.getVariable(), cfn))
  )
  or
  exists(LifetimeLocalScopeVariable lv | ps = PSetOwner(lv, _) |
    result = PSetInvalid(TVariableOutOfScope(lv.getVariable(), cfn))
    or
    exists(FunctionCall fc |
      fc = cfn and
      fc.getQualifier() = lv.getVariable().getAnAccess() and
      not fc.getTarget() instanceof ConstMemberFunction and
      // non-const versions of begin and end should nevertheless be considered const
      not fc.getTarget().hasName(["begin", "end"]) and
      result = PSetInvalid(TOwnerModified(fc))
    )
  )
  or
  // temporary objects end after the full expression
  exists(LifetimeTemporaryObject lto |
    ps = PSetVar(lto)
    or
    ps = PSetOwner(lto, _)
  |
    cfn = lto.getTemporaryObjectExpr().getUnconverted().getParent*().(FullExpr).getASuccessor() and
    result = PSetInvalid(TTemporaryOutOfScope(lto.getTemporaryObjectExpr()))
  )
}

/**
 * An expression which is dereferenced and may be an "invalid" value.
 */
class InvalidDereference extends VariableAccess {
  InvalidReason ir;

  InvalidDereference() {
    // The local points to map suggests this points to an invalid set
    exists(LocalScopeVariable lv |
      lv = this.getTarget() and
      pointsToMap(this, TLocalScopeVariable(lv), PSetInvalid(ir))
    )
  }

  /** Gets a reason why this dereference could point to an invalid value. */
  InvalidReason getAnInvalidReason() { result = ir }
}

from
  InvalidDereference e, Element explanation, string explanationDesc, InvalidReason ir,
  string invalidMessage
where
  ir = e.getAnInvalidReason() and
  ir.isAfterLifetime() and
  ir.hasMessage(invalidMessage, explanation, explanationDesc)
select e,
  e.(VariableAccess).getTarget().getName() + " is dereferenced here but accesses invalid memory " +
    invalidMessage, explanation, explanationDesc
