/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.0

import org.kde.latte.core 0.2 as LatteCore

import org.kde.latte.abilities.definition 0.1 as AbilityDefinition

AbilityDefinition.Indexer {
    id: _indexer
    property Item bridge: null
    property Item layout: null

    property bool updateIsBlocked: false

    readonly property bool isActive: bridge !== null
    readonly property bool inMarginsArea: isActive ? bridge.indexer.inMarginsArea : false
    readonly property bool tailAppletIsSeparator: isActive ? bridge.indexer.tailAppletIsSeparator : false
    readonly property bool headAppletIsSeparator: isActive ? bridge.indexer.headAppletIsSeparator : false
    readonly property bool isReady: !updateIsBlocked
    readonly property alias firstTailItemIsSeparator: _privates.firstTailItemIsSeparator
    readonly property alias lastHeadItemIsSeparator: _privates.lastHeadItemIsSeparator
    readonly property alias itemsCount: _privates.itemsCount
    readonly property alias visibleItemsCount: _privates.visibleItemsCount
    readonly property alias firstVisibleItemIndex: _privates.firstVisibleItemIndex
    readonly property alias lastVisibleItemIndex: _privates.lastVisibleItemIndex

    //! loop-safe bounds row for first/lastVisibleItemIndex. Those two
    //! properties feed TaskItem.isSeparatorHidden (only separators set
    //! it), so this collector must never read isSeparatorHidden of a
    //! separator or the binding chain loops back into itself - the Qt5
    //! bindings dodged the exact same loop by reading item properties
    //! with an isSeparator short-circuit instead of the hidden array.
    //! Separators are invisible to the bounds regardless, so their
    //! isHidden field stays false unread. Every other consumer uses the
    //! full rowEntries snapshot below, which is downstream of the bounds
    //! in the dependency order and may read isSeparatorHidden freely.
    property var boundsRowEntries: []

    QtObject {
        id: _privates
        property bool firstTailItemIsSeparator: false
        property bool lastHeadItemIsSeparator: false
        property int firstVisibleItemIndex: -1
        property int lastVisibleItemIndex: -1
        property int itemsCount: 0
        property int visibleItemsCount: 0
    }

    onIsActiveChanged: {
        if (isActive) {
            bridge.indexer.client = _indexer;
        }
    }

    Component.onCompleted: {
        if (isActive) {
            bridge.indexer.client = _indexer;
        }
    }

    Component.onDestruction: {
        if (isActive) {
            bridge.indexer.client = null;
        }
    }

    //! the collectors gather what the live children say; every verdict
    //! over them comes from org.kde.latte.core VisibleIndex (EX-06)

    Binding {
        target: _indexer
        property: "boundsRowEntries"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: {
            var row = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (!item || !(item.itemIndex>=0)) {
                    continue;
                }

                //! the isSeparator short-circuit is load-bearing (see the
                //! boundsRowEntries declaration comment)
                row.push({index: item.itemIndex,
                          isSeparator: item.isSeparator === true,
                          isHidden: !item.isSeparator && (item.isHidden || item.isSeparatorHidden) === true,
                          isMarginsSeparator: false,
                          subItemCount: 1});
            }

            return row;
        }
    }

    Binding {
        target: _indexer
        property: "rowEntries"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: {
            var row = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (!item || !(item.itemIndex>=0)) {
                    continue;
                }

                row.push({index: item.itemIndex,
                          isSeparator: item.isSeparator === true,
                          isHidden: (item.isHidden || item.isSeparatorHidden) === true,
                          isMarginsSeparator: false,
                          subItemCount: 1});
            }

            return row;
        }
    }

    Binding {
        target: _privates
        property: "firstTailItemIsSeparator"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: LatteCore.VisibleIndex.edgeItemIsSeparator(_indexer.rowEntries, LatteCore.VisibleIndex.Tail)
    }

    Binding {
        target: _privates
        property: "lastHeadItemIsSeparator"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: LatteCore.VisibleIndex.edgeItemIsSeparator(_indexer.rowEntries, LatteCore.VisibleIndex.Head)
    }

    Binding {
        target: _privates
        property: "firstVisibleItemIndex"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: LatteCore.VisibleIndex.firstVisibleIndex(_indexer.boundsRowEntries)
    }

    Binding {
        target: _privates
        property: "lastVisibleItemIndex"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: LatteCore.VisibleIndex.lastVisibleIndex(_indexer.boundsRowEntries)
    }

    Binding {
        target: _privates
        property: "visibleItemsCount"
        value: LatteCore.VisibleIndex.countVisibleItems(_indexer.rowEntries)
    }

    //! itemsCount is a plain collector count (children with an index),
    //! not index math - and unlike the guarded collectors above it never
    //! froze while updates were blocked, so it keeps its own loop
    Binding {
        target: _privates
        property: "itemsCount"
        value: {
            var count = 0;
            for(var i=0; i<_indexer.layout.children.length; ++i) {
                var item = _indexer.layout.children[i];
                if (item && item.itemIndex>=0) {
                    count = count + 1;
                }
            }

            return count;
        }
    }

    Binding {
        target: _indexer
        property: "hidden"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: {
            var hdns = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (item && (item.isHidden || item.isSeparatorHidden) && item.itemIndex>=0 && hdns.indexOf(item.itemIndex) < 0) {
                    hdns.push(item.itemIndex);
                }
            }

            return hdns;
        }
    }

    Binding {
        target: _indexer
        property: "separators"
        when: _indexer.isReady
        restoreMode: Binding.RestoreNone
        value: {
            var seps = [];

            for (var i=0; i<_indexer.layout.children.length; ++i){
                var item = _indexer.layout.children[i];
                if (item && item.isSeparator && item.itemIndex>=0 && seps.indexOf(item.itemIndex) < 0) {
                    seps.push(item.itemIndex);
                }
            }

            return seps;
        }
    }

    //! the host base is a live bridge read and is only asked for once the
    //! task is known to own a visible slot; a standalone client (no
    //! bridge) passes Qt5's -1 base and keeps the shipped answers
    function visibleIndex(taskIndex: int) : int {
        if (LatteCore.VisibleIndex.visibleIndexOf(_indexer.rowEntries, taskIndex) < 0) {
            return -1;
        }

        var hostBase = _indexer.bridge ? _indexer.bridge.indexer.host.visibleIndex(_indexer.bridge.indexer.appletIndex) : -1;

        return LatteCore.VisibleIndex.clientVisibleIndexOf(_indexer.rowEntries, taskIndex, hostBase);
    }
}
