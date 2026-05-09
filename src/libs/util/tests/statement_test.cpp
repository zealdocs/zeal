// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../sqlitedatabase.h"
#include "../statement.h"

#include <QtTest>

using namespace Zeal::Util;

class StatementTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void testInvalidSqlIsRejected();
    void testRejectsMultipleStatements();
    void testStatementOnUnopenedDatabase();
    void testStepReturnsFalseOnDone();
    void testValueByColumnIndex();
    void testBindText();
    void testBindInt();
    void testBindRebindsByDestroyAndRecreate();
    void testMultipleStatementsOnOneDb();
    void testTextWithApostrophe();
    void testEmptyTextBind();

    void testEscapeLikePattern();
    void testEscapeLikePatternIntegration();

private:
    SQLiteDatabase *m_db = nullptr;
};

void StatementTest::initTestCase()
{
    m_db = new SQLiteDatabase(QStringLiteral(":memory:"));
    QVERIFY(m_db->isOpen());
}

void StatementTest::init()
{
    QVERIFY(m_db->execute(QStringLiteral("DROP TABLE IF EXISTS t")));
    QVERIFY(m_db->execute(QStringLiteral("CREATE TABLE t (id INTEGER, name TEXT)")));
    QVERIFY(m_db->execute(QStringLiteral("INSERT INTO t VALUES (1, 'foo'), (2, 'bar'), (3, 'baz')")));
}

void StatementTest::testInvalidSqlIsRejected()
{
    Statement stmt(*m_db, QStringLiteral("NOT VALID SQL"));
    QVERIFY(!stmt.isValid());
    QVERIFY(!stmt.lastError().isEmpty());
}

void StatementTest::testRejectsMultipleStatements()
{
    Statement stmt(*m_db, QStringLiteral("SELECT 1; SELECT 2"));
    QVERIFY(!stmt.isValid());
    QVERIFY(!stmt.lastError().isEmpty());
}

void StatementTest::testStatementOnUnopenedDatabase()
{
    SQLiteDatabase bad(QStringLiteral("/non/existent/path/does/not/exist.db"));
    QVERIFY(!bad.isOpen());
    Statement stmt(bad, QStringLiteral("SELECT 1"));
    QVERIFY(!stmt.isValid());
    QVERIFY(!stmt.lastError().isEmpty());
}

void StatementTest::testStepReturnsFalseOnDone()
{
    Statement stmt(*m_db, QStringLiteral("SELECT id FROM t WHERE id = 999"));
    QVERIFY(stmt.isValid());
    QVERIFY(!stmt.step());
}

void StatementTest::testValueByColumnIndex()
{
    Statement stmt(*m_db, QStringLiteral("SELECT id, name FROM t WHERE id = 2"));
    QVERIFY(stmt.isValid());
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toInt(), 2);
    QCOMPARE(stmt.value(1).toString(), QStringLiteral("bar"));
    QVERIFY(!stmt.step());
}

void StatementTest::testBindText()
{
    Statement stmt(*m_db, QStringLiteral("SELECT id FROM t WHERE name = ?"));
    QVERIFY(stmt.isValid());
    stmt.bindText(1, QStringLiteral("baz"));
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toInt(), 3);
    QVERIFY(!stmt.step());
}

void StatementTest::testBindInt()
{
    Statement stmt(*m_db, QStringLiteral("SELECT name FROM t WHERE id = ?"));
    QVERIFY(stmt.isValid());
    stmt.bindInt(1, 1);
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toString(), QStringLiteral("foo"));
}

void StatementTest::testBindRebindsByDestroyAndRecreate()
{
    // Statements are short-lived RAII; "rebinding" means constructing a new
    // Statement. Verify that two sequential Statements on the same SQL with
    // different bound values yield independent results.
    QStringList names;
    for (int id : {1, 3}) {
        Statement stmt(*m_db, QStringLiteral("SELECT name FROM t WHERE id = ?"));
        QVERIFY(stmt.isValid());
        stmt.bindInt(1, id);
        QVERIFY(stmt.step());
        names << stmt.value(0).toString();
    }

    QCOMPARE(names, QStringList() << QStringLiteral("foo") << QStringLiteral("baz"));
}

void StatementTest::testMultipleStatementsOnOneDb()
{
    // Two live Statements on the same connection — no shared single-stmt slot.
    Statement a(*m_db, QStringLiteral("SELECT id FROM t ORDER BY id"));
    Statement b(*m_db, QStringLiteral("SELECT name FROM t ORDER BY id DESC"));
    QVERIFY(a.isValid());
    QVERIFY(b.isValid());

    QVERIFY(a.step());
    QCOMPARE(a.value(0).toInt(), 1);

    QVERIFY(b.step());
    QCOMPARE(b.value(0).toString(), QStringLiteral("baz"));

    QVERIFY(a.step());
    QCOMPARE(a.value(0).toInt(), 2);

    QVERIFY(b.step());
    QCOMPARE(b.value(0).toString(), QStringLiteral("bar"));
}

void StatementTest::testTextWithApostrophe()
{
    // Bind transparently handles single quotes — no manual ''-doubling needed.
    QVERIFY(m_db->execute(QStringLiteral("INSERT INTO t VALUES (4, 'O''Brien')")));

    Statement stmt(*m_db, QStringLiteral("SELECT id FROM t WHERE name = ?"));
    QVERIFY(stmt.isValid());
    stmt.bindText(1, QStringLiteral("O'Brien"));
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toInt(), 4);
}

void StatementTest::testEmptyTextBind()
{
    QVERIFY(m_db->execute(QStringLiteral("INSERT INTO t VALUES (5, '')")));

    Statement stmt(*m_db, QStringLiteral("SELECT id FROM t WHERE name = ?"));
    QVERIFY(stmt.isValid());
    stmt.bindText(1, QString());
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toInt(), 5);
}

void StatementTest::testEscapeLikePattern()
{
    QCOMPARE(escapeLikePattern(QStringLiteral("abc")), QStringLiteral("abc"));
    QCOMPARE(escapeLikePattern(QString()), QString());
    QCOMPARE(escapeLikePattern(QStringLiteral("100%")), QStringLiteral("100\\%"));
    QCOMPARE(escapeLikePattern(QStringLiteral("var_name")), QStringLiteral("var\\_name"));
    QCOMPARE(escapeLikePattern(QStringLiteral("a%b_c\\d")), QStringLiteral("a\\%b\\_c\\\\d"));
    QCOMPARE(escapeLikePattern(QStringLiteral("%%__\\\\")), QStringLiteral("\\%\\%\\_\\_\\\\\\\\"));
}

void StatementTest::testEscapeLikePatternIntegration()
{
    // The escaped pattern, paired with `ESCAPE '\\'`, must match the literal
    // string and not be treated as a wildcard.
    QVERIFY(m_db->execute(
        QStringLiteral("INSERT INTO t VALUES (10, '100%'), (11, '100X'), (12, 'var_name'), (13, 'varXname')")));

    Statement stmt(*m_db, QStringLiteral("SELECT id FROM t WHERE name LIKE ? ESCAPE '\\' ORDER BY id"));
    QVERIFY(stmt.isValid());
    stmt.bindText(1, escapeLikePattern(QStringLiteral("100%")));
    QVERIFY(stmt.step());
    QCOMPARE(stmt.value(0).toInt(), 10);
    QVERIFY(!stmt.step());
}

QTEST_MAIN(StatementTest)
#include "statement_test.moc"
