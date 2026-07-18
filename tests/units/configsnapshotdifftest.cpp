/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

// The pure diff core of the edit-mode settings audit's Tier B check
// (tests/units/configsnapshotdiff.h). This pins the acceptance property HC3
// asks of the harness: it must OBSERVE A REJECTION, never green-light a wrong
// outcome. Every rejection path is proven here deterministically -
//   - a stray side-effect write (the D15 minLength coupling) fails P2
//   - a no-change (the D10 dead-control class) fails P1
//   - the KConfig default-deletion trap surfaces as a LOUD diff, never a
//     silent pass
// so the ~dozen suspected-broken controls the audit hunts cannot slip through
// a harness that only passes the happy path.

#include "configsnapshotdiff.h"

#include <QJsonObject>
#include <QTest>

using namespace Latte::AuditHarness;

class ConfigSnapshotDiffTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void changedKeysAreSortedAndDeduplicated();
    void unchangedSnapshotHasNoChangedKeys();
    void appliesDetectsTheDrivenKey();

    //! HC3: the harness catches a wrong outcome
    void noChangeFailsApplies();
    void strayCoupledWriteFailsRightKeyOnly();
    void exactExpectedSetPassesRightKeyOnly();
    void missingExpectedKeyFailsRightKeyOnly();
    void expectedSetOrderAndDuplicatesDoNotMatter();

    //! default-deletion safety: absent-on-one-side is a loud diff
    void keyRemovedIsAChangeNotSilentPass();
    void keyAddedIsAChange();

    //! P3 reflect-state
    void valueReflectsMatchesAndMisses();
};

//! a config snapshot's "config" object, built from key/value pairs
static QJsonObject cfg(std::initializer_list<std::pair<QString, QJsonValue>> pairs)
{
    QJsonObject json;
    for (const auto &pair : pairs) {
        json.insert(pair.first, pair.second);
    }
    return json;
}

void ConfigSnapshotDiffTest::changedKeysAreSortedAndDeduplicated()
{
    const QJsonObject before = cfg({{QStringLiteral("offset"), 0}, {QStringLiteral("maxLength"), 100}, {QStringLiteral("minLength"), 30}});
    const QJsonObject after = cfg({{QStringLiteral("offset"), 5}, {QStringLiteral("maxLength"), 90}, {QStringLiteral("minLength"), 30}});

    //! two keys moved, reported sorted and each once
    QCOMPARE(changedConfigKeys(before, after), (QStringList{QStringLiteral("maxLength"), QStringLiteral("offset")}));
}

void ConfigSnapshotDiffTest::unchangedSnapshotHasNoChangedKeys()
{
    const QJsonObject snap = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("minLength"), 30}});

    QVERIFY(changedConfigKeys(snap, snap).isEmpty());
}

void ConfigSnapshotDiffTest::appliesDetectsTheDrivenKey()
{
    const QJsonObject before = cfg({{QStringLiteral("iconSize"), 48}});
    const QJsonObject after = cfg({{QStringLiteral("iconSize"), 64}});

    QVERIFY(controlApplies(before, after, QStringLiteral("iconSize")));
}

//! A control that writes a key nothing reads (or writes nothing at all) leaves
//! the snapshot unchanged: P1 must FAIL, exactly the D10 dead-control shape.
void ConfigSnapshotDiffTest::noChangeFailsApplies()
{
    const QJsonObject snap = cfg({{QStringLiteral("titleTooltips"), false}, {QStringLiteral("maxLength"), 100}});

    QVERIFY2(!controlApplies(snap, snap, QStringLiteral("titleTooltips")),
             "a control that changed nothing must FAIL the P1 applies check");
}

//! The D15 shape: driving the Maximum ruler also moved minLength. P2 with the
//! expected set {maxLength} must FAIL because a stray key changed too.
void ConfigSnapshotDiffTest::strayCoupledWriteFailsRightKeyOnly()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("minLength"), 100}, {QStringLiteral("offset"), 0}});
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 90}, {QStringLiteral("minLength"), 90}, {QStringLiteral("offset"), 0}});

    QVERIFY2(!onlyExpectedKeysChanged(before, after, {QStringLiteral("maxLength")}),
             "a coupled minLength side effect must FAIL the P2 right-key-only check");
    //! and the diff names the stray, so the audit can report WHICH key leaked
    QCOMPARE(changedConfigKeys(before, after), (QStringList{QStringLiteral("maxLength"), QStringLiteral("minLength")}));
}

void ConfigSnapshotDiffTest::exactExpectedSetPassesRightKeyOnly()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("minLength"), 30}});
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 90}, {QStringLiteral("minLength"), 30}});

    QVERIFY(onlyExpectedKeysChanged(before, after, {QStringLiteral("maxLength")}));
}

void ConfigSnapshotDiffTest::missingExpectedKeyFailsRightKeyOnly()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("offset"), 0}});
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 90}, {QStringLiteral("offset"), 0}});

    //! expected two keys, only one moved: still a FAIL (an under-write is as
    //! wrong as an over-write)
    QVERIFY2(!onlyExpectedKeysChanged(before, after, {QStringLiteral("maxLength"), QStringLiteral("offset")}),
             "an expected key that did NOT change must FAIL the P2 check");
}

void ConfigSnapshotDiffTest::expectedSetOrderAndDuplicatesDoNotMatter()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("offset"), 0}});
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 90}, {QStringLiteral("offset"), 5}});

    //! the caller may pass keys unsorted or repeated; normalization makes P2
    //! order- and duplicate-insensitive
    QVERIFY(onlyExpectedKeysChanged(before, after, {QStringLiteral("offset"), QStringLiteral("maxLength"), QStringLiteral("offset")}));
}

//! The KConfig default-deletion trap made safe: a key present before and gone
//! after (as an on-disk file would drop a defaulted key) is a LOUD change, not
//! a swallowed "no change". This is the false-FAIL direction HC3 mandates.
void ConfigSnapshotDiffTest::keyRemovedIsAChangeNotSilentPass()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("minLength"), 30}});
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 100}});

    QVERIFY2(changedConfigKeys(before, after).contains(QStringLiteral("minLength")),
             "a key that vanished from the after-snapshot must surface as a change, never a silent pass");
    QVERIFY(controlApplies(before, after, QStringLiteral("minLength")));
}

void ConfigSnapshotDiffTest::keyAddedIsAChange()
{
    const QJsonObject before = cfg({{QStringLiteral("maxLength"), 100}});
    //! S-a shape: a control wrote a key that was not there before (a
    //! schema-absent stray landing in the map)
    const QJsonObject after = cfg({{QStringLiteral("maxLength"), 100}, {QStringLiteral("solidPanel"), true}});

    QCOMPARE(changedConfigKeys(before, after), (QStringList{QStringLiteral("solidPanel")}));
}

void ConfigSnapshotDiffTest::valueReflectsMatchesAndMisses()
{
    const QJsonObject snap = cfg({{QStringLiteral("alignment"), 10}, {QStringLiteral("blurEnabled"), true}});

    QVERIFY(valueReflects(snap, QStringLiteral("alignment"), 10));
    QVERIFY(valueReflects(snap, QStringLiteral("blurEnabled"), true));
    //! wrong value fails
    QVERIFY(!valueReflects(snap, QStringLiteral("alignment"), 20));
    //! absent key fails (never a silent match on a missing key)
    QVERIFY(!valueReflects(snap, QStringLiteral("offset"), 0));
}

QTEST_GUILESS_MAIN(ConfigSnapshotDiffTest)

#include "configsnapshotdifftest.moc"
