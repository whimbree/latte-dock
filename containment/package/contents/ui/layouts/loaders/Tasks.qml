/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7

import org.kde.taskmanager 0.1 as TaskManager

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.private.containment 0.1 as LatteContainment

Loader {
    id: tasksLoader
    active: root.scrollAction === LatteContainment.Types.ScrollTasks || root.scrollAction === LatteContainment.Types.ScrollToggleMinimized
    sourceComponent: Item {
        TaskManager.TasksModel {
            id: tasksModel
            virtualDesktop: virtualDesktopInfo.currentDesktop
            screenGeometry: latteView ? latteView.screenGeometry : Qt.rect(-1, -1, 0, 0)
            activity: activityInfo.currentActivity

            filterByVirtualDesktop: true
            filterByScreen:latteView ?  true : false
            filterByActivity: true

            launchInPlace: true
            separateLaunchers: true
            groupInline: false

            groupMode: TaskManager.TasksModel.GroupApplications
            sortMode: TaskManager.TasksModel.SortManual
        }

        TaskManager.VirtualDesktopInfo {
            id: virtualDesktopInfo
        }

        TaskManager.ActivityInfo {
            id: activityInfo
        }

        Item{
            id: taskList
            Repeater{
                model: tasksModel
                Item{
                    readonly property var m: model

                    function modelIndex() {
                        return tasksModel.makeModelIndex(index);
                    }
                }
            }
        }

        //! Thin shell over LatteCore.WindowCycler (units/windowcycler.h;
        //! plasmoid tools.js activateNextPrevTask is its twin): the core
        //! owns the launcher/startup filtering, group expansion and
        //! wraparound choice; the live parts kept here are the model-row
        //! mirror walk (taskList's Repeater tracks tasksModel in model
        //! order, its last child is the Repeater itself - hence length - 1),
        //! the model-index construction and the activeTask identity match.
        function activateNextPrevTask(next) {
            var entries = [];

            for (var i = 0; i < taskList.children.length - 1; ++i) {
                var task = taskList.children[i];
                var isGroupParent = task.m.IsGroupParent === true;

                entries.push({
                    isLauncher: task.m.IsLauncher === true,
                    isStartup: task.m.IsStartup === true,
                    isGroupParent: isGroupParent,
                    childCount: isGroupParent ? tasksModel.rowCount(task.modelIndex()) : 0
                });
            }

            var positions = LatteCore.WindowCycler.flattenTasksForCycling(entries);
            var activeTaskIndex = tasksModel.activeTask;
            var taskIndexList = [];
            var activeAt = -1;

            for (var p = 0; p < positions.length; ++p) {
                var modelIndex = (positions[p].childRow >= 0
                        ? tasksModel.makeModelIndex(positions[p].row, positions[p].childRow)
                        : tasksModel.makeModelIndex(positions[p].row));

                if (modelIndex === activeTaskIndex) {
                    activeAt = p;
                }

                taskIndexList.push(modelIndex);
            }

            if (!taskIndexList.length) {
                //! a bar of launchers only: nothing to cycle (Qt5 behavior)
                return;
            }

            var target = LatteCore.WindowCycler.selectAdjacentTask(taskIndexList.length, activeAt, next);

            if (target < 0) {
                //! only reachable through the wrapper's malformed-input
                //! refusal, which already reported the bug loudly
                return;
            }

            tasksModel.requestActivate(taskIndexList[target]);
        }
    }
}
