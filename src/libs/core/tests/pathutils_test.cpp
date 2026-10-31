// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../pathutils.h"

#include <QtTest>

using Zeal::Core::Detail::expandTildePath;

class PathUtilsTest : public QObject
{
    Q_OBJECT

private slots:
    void expandsBareTilde();
    void expandsHomeRelativePath();
    void avoidsDuplicateSeparators();
    void leavesOtherPathsUnchanged();
    void leavesNamedHomePathsUnchanged();
    void leavesTildePathUnchangedWithoutHomePath();
};

void PathUtilsTest::expandsBareTilde()
{
#if defined(Q_OS_UNIX)
    QCOMPARE(expandTildePath(QStringLiteral("~"), QStringLiteral("/home/zeal")), QStringLiteral("/home/zeal"));
#else
    QCOMPARE(expandTildePath(QStringLiteral("~"), QStringLiteral("/home/zeal")), QStringLiteral("~"));
#endif
}

void PathUtilsTest::expandsHomeRelativePath()
{
#if defined(Q_OS_UNIX)
    QCOMPARE(expandTildePath(QStringLiteral("~/docsets"), QStringLiteral("/home/zeal")),
             QStringLiteral("/home/zeal/docsets"));
#else
    QCOMPARE(expandTildePath(QStringLiteral("~/docsets"), QStringLiteral("/home/zeal")), QStringLiteral("~/docsets"));
#endif
}

void PathUtilsTest::avoidsDuplicateSeparators()
{
#if defined(Q_OS_UNIX)
    QCOMPARE(expandTildePath(QStringLiteral("~/docsets"), QStringLiteral("/")), QStringLiteral("/docsets"));
    QCOMPARE(expandTildePath(QStringLiteral("~/docsets"), QStringLiteral("/home/zeal/")),
             QStringLiteral("/home/zeal/docsets"));
#else
    QCOMPARE(expandTildePath(QStringLiteral("~/docsets"), QStringLiteral("/")), QStringLiteral("~/docsets"));
#endif
}

void PathUtilsTest::leavesOtherPathsUnchanged()
{
    const QString path = QStringLiteral("docsets");
    QCOMPARE(expandTildePath(path, QStringLiteral("/home/zeal")), path);
}

void PathUtilsTest::leavesNamedHomePathsUnchanged()
{
    const QString path = QStringLiteral("~another-user/docsets");
    QCOMPARE(expandTildePath(path, QStringLiteral("/home/zeal")), path);
}

void PathUtilsTest::leavesTildePathUnchangedWithoutHomePath()
{
    const QString path = QStringLiteral("~/docsets");
    QCOMPARE(expandTildePath(path, QString()), path);
}

QTEST_GUILESS_MAIN(PathUtilsTest)

#include "pathutils_test.moc"
