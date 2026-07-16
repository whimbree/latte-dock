# Parabolic chain equality harness (EX-02 generator)

Drives the REAL shipped propagation chain offscreen (abilities definition
ParabolicEffect hub + containment ParabolicArea deciders + real
ParabolicEdgeSpacer inside per-item mock context scopes) and prints one
CASE| line per scenario. The vectors baked into
tests/units/parabolicroutertest.cpp came from this harness at 0613c2ae:

    scripts/qml-interaction-tests.sh tests/generators/parabolicchain

NOT wired into ctest: it is a generation tool, and after the EX-02
cutover deletes the QML deciders it documents where the reference
vectors came from rather than remaining runnable against them. Re-run
it BEFORE the cutover if the case matrix ever needs extending.
