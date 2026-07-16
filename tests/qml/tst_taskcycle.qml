/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Pins the plasmoid's whole-bar task cycle (EX-16: tools.js
//! activateNextPrevTask, the wheel action behind the wheelEnabled
//! interaction option). Written FAILING-FIRST against the shipped body,
//! which referenced taskList - an id that exists nowhere in the plasmoid's
//! context chain (the list is icList; taskList is plasma-desktop applet
//! heritage) - so every wheel cycle threw ReferenceError since Qt5 and the
//! option silently did nothing. Green with the fix; stays green across the
//! EX-16 cutover as the twin-equivalence evidence.
//!
//! Context plumbing: tools.js resolves its names through this document's
//! ids exactly as it resolves them through main.qml's in production -
//! tasksModel is a recorder facade, icList.contentItem carries
//! TaskItem-shaped children (objectName "TaskItem", itemIndex, the typed
//! isLauncher/isStartup/isGroupParent, modelIndex()) plus a chrome child
//! the assembly must skip. Children are created in SCRAMBLED order on
//! purpose: a ListView's contentItem holds delegates in creation order,
//! not model order, and the fix orders by itemIndex - the regression case
//! for that is test_creationOrderDoesNotLeakIntoCycleOrder. Model indexes
//! travel as string tokens so === keeps value semantics.

import QtQuick 2.7
import QtTest 1.2

import "../../plasmoid/package/contents/code/tools.js" as TaskTools

Item {
    id: root
    width: 400
    height: 200

    property bool vertical: false

    QtObject {
        id: tasksModel

        //! string token of the active task's model index ("" = none)
        property var activeTask: ""
        property var activateRequests: []
        //! per-row child counts for group parents, keyed by row
        property var groupChildCounts: ({})

        function makeModelIndex(row, childRow) {
            return childRow === undefined ? "t" + row : row + ":" + childRow;
        }
        function rowCount(idx) {
            var row = parseInt(String(idx).replace("t", ""));
            return groupChildCounts[row] !== undefined ? groupChildCounts[row] : 0;
        }
        function requestActivate(idx) {
            activateRequests.push(idx);
        }
    }

    Component {
        id: fakeTaskComponent

        Item {
            objectName: "TaskItem"
            property int itemIndex: 0
            property bool isLauncher: false
            property bool isStartup: false
            property bool isGroupParent: false
            //! reads the shipped body performed through task.m stay
            //! honest for whichever body is live
            property var m: ({
                IsLauncher: isLauncher,
                IsStartup: isStartup,
                IsGroupParent: isGroupParent
            })
            function modelIndex() {
                return tasksModel.makeModelIndex(itemIndex);
            }
        }
    }

    Item {
        id: icList
        property Item contentItem: listContent

        Item {
            id: listContent
        }
    }

    TestCase {
        name: "TaskCycle"
        when: windowShown

        //! rows: [{launcher, startup, group, children}] in MODEL order;
        //! creationOrder permutes delegate creation to mimic a ListView's
        //! creation-ordered contentItem
        function setTasks(rows, creationOrder) {
            for (var i = listContent.children.length - 1; i >= 0; --i) {
                listContent.children[i].destroy();
            }
            //! destroy() completes through the event loop
            tryVerify(function() { return listContent.children.length === 0; });

            tasksModel.activateRequests = [];
            tasksModel.activeTask = "";
            tasksModel.groupChildCounts = {};

            var order = creationOrder !== undefined
                    ? creationOrder
                    : Array.from({ length: rows.length }, function(v, k) { return k; });

            //! a ListView's contentItem also carries chrome the assembly
            //! must skip; a nameless child stands in for it
            Qt.createQmlObject("import QtQuick 2.7; Item {}", listContent);

            for (var c = 0; c < order.length; ++c) {
                var row = order[c];
                var spec = rows[row];
                fakeTaskComponent.createObject(listContent, {
                    itemIndex: row,
                    isLauncher: spec.launcher === true,
                    isStartup: spec.startup === true,
                    isGroupParent: spec.group === true
                });
                if (spec.group === true) {
                    tasksModel.groupChildCounts[row] = spec.children;
                }
            }
        }

        function test_nextActivatesFollowingTask() {
            setTasks([{}, {}, {}]);
            tasksModel.activeTask = "t1";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t2"]);
        }

        function test_nextAtEndWrapsToFirst() {
            setTasks([{}, {}, {}]);
            tasksModel.activeTask = "t2";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t0"]);
        }

        function test_previousActivatesPrecedingTask() {
            setTasks([{}, {}, {}]);
            tasksModel.activeTask = "t2";
            TaskTools.activateNextPrevTask(false);
            compare(tasksModel.activateRequests, ["t1"]);
        }

        function test_previousAtStartWrapsToLast() {
            setTasks([{}, {}, {}]);
            tasksModel.activeTask = "t0";
            TaskTools.activateNextPrevTask(false);
            compare(tasksModel.activateRequests, ["t2"]);
        }

        function test_activeNotInListFallsToFirst() {
            setTasks([{}, {}]);
            tasksModel.activeTask = "elsewhere";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t0"]);
        }

        function test_launchersAndStartupsDropOutOfTheCycle() {
            setTasks([{ launcher: true }, {}, { startup: true }, {}]);
            tasksModel.activeTask = "t1";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t3"]);
        }

        function test_groupParentContributesOneStopPerChildWindow() {
            setTasks([{}, { group: true, children: 3 }, {}]);
            tasksModel.activeTask = "1:1";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["1:2"]);
        }

        function test_nextLeavesGroupAfterLastChild() {
            setTasks([{}, { group: true, children: 2 }, {}]);
            tasksModel.activeTask = "1:1";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t2"]);
        }

        function test_onlyLaunchersMeansNothingToCycle() {
            setTasks([{ launcher: true }, { launcher: true }]);
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, []);
        }

        function test_emptyBarMeansNothingToCycle() {
            setTasks([]);
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, []);
        }

        //! the fix's regression case: delegates created 2,0,1 must still
        //! cycle 0 -> 1 -> 2 (bar order), not creation order
        function test_creationOrderDoesNotLeakIntoCycleOrder() {
            setTasks([{}, {}, {}], [2, 0, 1]);
            tasksModel.activeTask = "t0";
            TaskTools.activateNextPrevTask(true);
            compare(tasksModel.activateRequests, ["t1"]);
        }
    }
}
