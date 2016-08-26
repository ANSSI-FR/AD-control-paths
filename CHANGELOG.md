# ChangeLog

## [1.2] - 2016-08-26
* Massive code refactoring through dedicated utils libs
* New ldap dumper allowing scalability
* UNICODE settings, tchar everything
* Admincount processing
* LAPS processing
* Inherited ACEs are not filtered out any more: per-object evaluation instead, fixing some errors
* Rewrite some allocator functions and macros, fixing leaks
* Deny ACEs dump, filtering and post-processing
* Drop the custom java importer, neo4j 3 CSV batch importer used instead
* Bugfixes

## [1.1] - 2016-03-02
* Fix ObjectType false positives
* Rewrite of object class code
* Binaries now packaged in releases, placeholders instead in source tree

## [0.1] - 2014-07-23
* Initial Release
