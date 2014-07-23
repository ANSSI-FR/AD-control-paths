#ifndef __AVL_TABLE__
#define __AVL_TABLE__

//
// Taken from <ntddk.h>
// Rtl*GenericTableAvl functions are also present in userland
// They are exported by ntdll.dll
//

// This 2 lines only is from <ntdef.h>
#define NTAPI __stdcall
typedef ULONG CLONG;

#ifdef __cplusplus
extern "C" {
#endif


    typedef enum _TABLE_SEARCH_RESULT{
        TableEmptyTree,
        TableFoundNode,
        TableInsertAsLeft,
        TableInsertAsRight
    } TABLE_SEARCH_RESULT;

    //
    //  The results of a compare can be less than, equal, or greater than.
    //

    typedef enum _RTL_GENERIC_COMPARE_RESULTS {
        GenericLessThan,
        GenericGreaterThan,
        GenericEqual
    } RTL_GENERIC_COMPARE_RESULTS;

    //
    //  Define the Avl version of the generic table package.  Note a generic table
    //  should really be an opaque type.  We provide routines to manipulate the structure.
    //
    //  A generic table is package for inserting, deleting, and looking up elements
    //  in a table (e.g., in a symbol table).  To use this package the user
    //  defines the structure of the elements stored in the table, provides a
    //  comparison function, a memory allocation function, and a memory
    //  deallocation function.
    //
    //  Note: the user compare function must impose a complete ordering among
    //  all of the elements, and the table does not allow for duplicate entries.
    //

    //
    // Add an empty typedef so that functions can reference the
    // a pointer to the generic table struct before it is declared.
    //

    struct _RTL_AVL_TABLE;

    //
    //  The comparison function takes as input pointers to elements containing
    //  user defined structures and returns the results of comparing the two
    //  elements.
    //

    typedef
        _IRQL_requires_same_
        _Function_class_(RTL_AVL_COMPARE_ROUTINE)
        RTL_GENERIC_COMPARE_RESULTS
        NTAPI
        RTL_AVL_COMPARE_ROUTINE(
        _In_ struct _RTL_AVL_TABLE *Table,
        _In_ PVOID FirstStruct,
        _In_ PVOID SecondStruct
        );
    typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;

    //
    //  The allocation function is called by the generic table package whenever
    //  it needs to allocate memory for the table.
    //

    typedef
        _IRQL_requires_same_
        _Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
        __drv_allocatesMem(Mem)
        PVOID
        NTAPI
        RTL_AVL_ALLOCATE_ROUTINE(
        _In_ struct _RTL_AVL_TABLE *Table,
        _In_ CLONG ByteSize
        );
    typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

    //
    //  The deallocation function is called by the generic table package whenever
    //  it needs to deallocate memory from the table that was allocated by calling
    //  the user supplied allocation function.
    //

    typedef
        _IRQL_requires_same_
        _Function_class_(RTL_AVL_FREE_ROUTINE)
        VOID
        NTAPI
        RTL_AVL_FREE_ROUTINE(
        _In_ struct _RTL_AVL_TABLE *Table,
        _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
        );
    typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;

    //
    //  The match function takes as input the user data to be matched and a pointer
    //  to some match data, which was passed along with the function pointer.  It
    //  returns TRUE for a match and FALSE for no match.
    //
    //  RTL_AVL_MATCH_FUNCTION returns
    //      STATUS_SUCCESS if the IndexRow matches
    //      STATUS_NO_MATCH if the IndexRow does not match, but the enumeration should
    //          continue
    //      STATUS_NO_MORE_MATCHES if the IndexRow does not match, and the enumeration
    //          should terminate
    //


    typedef
        _IRQL_requires_same_
        _Function_class_(RTL_AVL_MATCH_FUNCTION)
        NTSTATUS
        NTAPI
        RTL_AVL_MATCH_FUNCTION(
        _In_ struct _RTL_AVL_TABLE *Table,
        _In_ PVOID UserData,
        _In_ PVOID MatchData
        );
    typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

    //
    //  Define the balanced tree links and Balance field.  (No Rank field
    //  defined at this time.)
    //
    //  Callers should treat this structure as opaque!
    //
    //  The root of a balanced binary tree is not a real node in the tree
    //  but rather points to a real node which is the root.  It is always
    //  in the table below, and its fields are used as follows:
    //
    //      Parent      Pointer to self, to allow for detection of the root.
    //      LeftChild   NULL
    //      RightChild  Pointer to real root
    //      Balance     Undefined, however it is set to a convenient value
    //                  (depending on the algorithm) prior to rebalancing
    //                  in insert and delete routines.
    //

    typedef struct _RTL_BALANCED_LINKS {
        struct _RTL_BALANCED_LINKS *Parent;
        struct _RTL_BALANCED_LINKS *LeftChild;
        struct _RTL_BALANCED_LINKS *RightChild;
        CHAR Balance;
        UCHAR Reserved[3];
    } RTL_BALANCED_LINKS;
    typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;


    //
    //  To use the generic table package the user declares a variable of type
    //  GENERIC_TABLE and then uses the routines described below to initialize
    //  the table and to manipulate the table.  Note that the generic table
    //  should really be an opaque type.
    //

    typedef struct _RTL_AVL_TABLE {
        RTL_BALANCED_LINKS BalancedRoot;
        PVOID OrderedPointer;
        ULONG WhichOrderedElement;
        ULONG NumberGenericTableElements;
        ULONG DepthOfTree;
        PRTL_BALANCED_LINKS RestartKey;
        ULONG DeleteCount;
        PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
        PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
        PRTL_AVL_FREE_ROUTINE FreeRoutine;
        PVOID TableContext;
    } RTL_AVL_TABLE;
    typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

    //
    //  The procedure InitializeGenericTable takes as input an uninitialized
    //  generic table variable and pointers to the three user supplied routines.
    //  This must be called for every individual generic table variable before
    //  it can be used.
    //

    //#if (NTDDI_VERSION >= NTDDI_WINXP)
    typedef NTSYSAPI
        VOID
        (NTAPI *RTLINITIALIZEGENERICTABLEAVL) (
        _Out_ PRTL_AVL_TABLE Table,
        _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
        _In_opt_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
        _In_opt_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
        _In_opt_ PVOID TableContext
        );
    //#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function InsertElementGenericTable will insert a new element
    //  in a table.  It does this by allocating space for the new element
    //  (this includes AVL links), inserting the element in the table, and
    //  then returning to the user a pointer to the new element.  If an element
    //  with the same key already exists in the table the return value is a pointer
    //  to the old element.  The optional output parameter NewElement is used
    //  to indicate if the element previously existed in the table.  Note: the user
    //  supplied Buffer is only used for searching the table, upon insertion its
    //  contents are copied to the newly created element.  This means that
    //  pointer to the input buffer will not point to the new element.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    typedef NTSYSAPI
        PVOID
        (NTAPI *RTLINSERTELEMENTGENERICTABLEAVL) (
        _In_ PRTL_AVL_TABLE Table,
        _In_reads_bytes_(BufferSize) PVOID Buffer,
        _In_ CLONG BufferSize,
        _Out_opt_ PBOOLEAN NewElement
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function InsertElementGenericTableFull will insert a new element
    //  in a table.  It does this by allocating space for the new element
    //  (this includes AVL links), inserting the element in the table, and
    //  then returning to the user a pointer to the new element.  If an element
    //  with the same key already exists in the table the return value is a pointer
    //  to the old element.  The optional output parameter NewElement is used
    //  to indicate if the element previously existed in the table.  Note: the user
    //  supplied Buffer is only used for searching the table, upon insertion its
    //  contents are copied to the newly created element.  This means that
    //  pointer to the input buffer will not point to the new element.
    //  This routine is passed the NodeOrParent and SearchResult from a
    //  previous RtlLookupElementGenericTableFull.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    NTSYSAPI
        PVOID
        NTAPI
        RtlInsertElementGenericTableFullAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_reads_bytes_(BufferSize) PVOID Buffer,
        _In_ CLONG BufferSize,
        _Out_opt_ PBOOLEAN NewElement,
        _In_ PVOID NodeOrParent,
        _In_ TABLE_SEARCH_RESULT SearchResult
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function DeleteElementGenericTable will find and delete an element
    //  from a generic table.  If the element is located and deleted the return
    //  value is TRUE, otherwise if the element is not located the return value
    //  is FALSE.  The user supplied input buffer is only used as a key in
    //  locating the element in the table.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    typedef NTSYSAPI
        BOOLEAN
        (NTAPI *RTLDELETEGENERICTABLEAVL)(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID Buffer
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function DeleteElementGenericTableAvxEx deletes the element specified
    //  by the NodeOrParent pointer. This element user data pointer must have first
    //  been obtained with RtlLookupElementGenericTableFull.
    //

#if (NTDDI_VERSION >= NTDDI_WIN8)
    NTSYSAPI
        VOID
        NTAPI
        RtlDeleteElementGenericTableAvlEx(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID NodeOrParent
        );
#endif // NTDDI_VERSION >= NTDDI_WIN8


    //
    //  The function LookupElementGenericTable will find an element in a generic
    //  table.  If the element is located the return value is a pointer to
    //  the user defined structure associated with the element, otherwise if
    //  the element is not located the return value is NULL.  The user supplied
    //  input buffer is only used as a key in locating the element in the table.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        typedef NTSYSAPI
        PVOID
        (NTAPI *RTLLOOKUPELEMENTGENERICTABLEAVL) (
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID Buffer
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function LookupElementGenericTableFull will find an element in a generic
    //  table.  If the element is located the return value is a pointer to
    //  the user defined structure associated with the element.  If the element is not
    //  located then a pointer to the parent for the insert location is returned.  The
    //  user must look at the SearchResult value to determine which is being returned.
    //  The user can use the SearchResult and parent for a subsequent FullInsertElement
    //  call to optimize the insert.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    NTSYSAPI
        PVOID
        NTAPI
        RtlLookupElementGenericTableFullAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID Buffer,
        _Out_ PVOID *NodeOrParent,
        _Out_ TABLE_SEARCH_RESULT *SearchResult
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function EnumerateGenericTable will return to the caller one-by-one
    //  the elements of of a table.  The return value is a pointer to the user
    //  defined structure associated with the element.  The input parameter
    //  Restart indicates if the enumeration should start from the beginning
    //  or should return the next element.  If the are no more new elements to
    //  return the return value is NULL.  As an example of its use, to enumerate
    //  all of the elements in a table the user would write:
    //
    //      for (ptr = EnumerateGenericTable(Table, TRUE);
    //           ptr != NULL;
    //           ptr = EnumerateGenericTable(Table, FALSE)) {
    //              :
    //      }
    //
    //  NOTE:   This routine does not modify the structure of the tree, but saves
    //          the last node returned in the generic table itself, and for this
    //          reason requires exclusive access to the table for the duration of
    //          the enumeration.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        typedef NTSYSAPI
        PVOID
        (NTAPI *RTLENUMERATEGENERICTABLEAVL) (
        _In_ PRTL_AVL_TABLE Table,
        _In_ BOOLEAN Restart
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function EnumerateGenericTableWithoutSplaying will return to the
    //  caller one-by-one the elements of of a table.  The return value is a
    //  pointer to the user defined structure associated with the element.
    //  The input parameter RestartKey indicates if the enumeration should
    //  start from the beginning or should return the next element.  If the
    //  are no more new elements to return the return value is NULL.  As an
    //  example of its use, to enumerate all of the elements in a table the
    //  user would write:
    //
    //      RestartKey = NULL;
    //      for (ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey);
    //           ptr != NULL;
    //           ptr = EnumerateGenericTableWithoutSplaying(Table, &RestartKey)) {
    //              :
    //      }
    //
    //  If RestartKey is NULL, the package will start from the least entry in the
    //  table, otherwise it will start from the last entry returned.
    //
    //  NOTE:   This routine does not modify either the structure of the tree
    //          or the generic table itself, but must insure that no deletes
    //          occur for the duration of the enumeration, typically by having
    //          at least shared access to the table for the duration.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        NTSYSAPI
        PVOID
        NTAPI
        RtlEnumerateGenericTableWithoutSplayingAvl(
        _In_ PRTL_AVL_TABLE Table,
        _Inout_ PVOID *RestartKey
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  RtlLookupFirstMatchingElementGenericTableAvl will return the left-most
    //  element in the tree matching the data in Buffer.  If, for example, the tree
    //  contains filenames there may exist several that differ only in case. A case-
    //  blind searcher can use this routine to position himself in the tree at the
    //  first match, and use an enumeration routine (such as RtlEnumerateGenericTableWithoutSplayingAvl
    //  to return each subsequent match.
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        NTSYSAPI
        PVOID
        NTAPI
        RtlLookupFirstMatchingElementGenericTableAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID Buffer,
        _Out_ PVOID *RestartKey
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function EnumerateGenericTableLikeADirectory will return to the
    //  caller one-by-one the elements of of a table.  The return value is a
    //  pointer to the user defined structure associated with the element.
    //  The input parameter RestartKey indicates if the enumeration should
    //  start from the beginning or should return the next element.  If the
    //  are no more new elements to return the return value is NULL.  As an
    //  example of its use, to enumerate all of the elements in a table the
    //  user would write:
    //
    //      RestartKey = NULL;
    //      for (ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...);
    //           ptr != NULL;
    //           ptr = EnumerateGenericTableLikeADirectory(Table, &RestartKey, ...)) {
    //              :
    //      }
    //
    //  If RestartKey is NULL, the package will start from the least entry in the
    //  table, otherwise it will start from the last entry returned.
    //
    //  NOTE:   This routine does not modify either the structure of the tree
    //          or the generic table itself.  The table must only be acquired
    //          shared for the duration of this call, and all synchronization
    //          may optionally be dropped between calls.  Enumeration is always
    //          correctly resumed in the most efficient manner possible via the
    //          IN OUT parameters provided.
    //
    //  ******  Explain NextFlag.  Directory enumeration resumes from a key
    //          requires more thought.  Also need the match pattern and IgnoreCase.
    //          Should some structure be introduced to carry it all?
    //

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        NTSYSAPI
        PVOID
        NTAPI
        RtlEnumerateGenericTableLikeADirectory(
        _In_ PRTL_AVL_TABLE Table,
        _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
        _In_opt_ PVOID MatchData,
        _In_ ULONG NextFlag,
        _Inout_ PVOID *RestartKey,
        _Inout_ PULONG DeleteCount,
        _In_ PVOID Buffer
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    // The function GetElementGenericTable will return the i'th element
    // inserted in the generic table.  I = 0 implies the first element,
    // I = (RtlNumberGenericTableElements(Table)-1) will return the last element
    // inserted into the generic table.  The type of I is ULONG.  Values
    // of I > than (NumberGenericTableElements(Table)-1) will return NULL.  If
    // an arbitrary element is deleted from the generic table it will cause
    // all elements inserted after the deleted element to "move up".

#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        NTSYSAPI
        PVOID
        NTAPI
        RtlGetElementGenericTableAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ ULONG I
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    // The function NumberGenericTableElements returns a ULONG value
    // which is the number of generic table elements currently inserted
    // in the generic table.

#if (NTDDI_VERSION >= NTDDI_WINXP)
    NTSYSAPI
        ULONG
        NTAPI
        RtlNumberGenericTableElementsAvl(
        _In_ PRTL_AVL_TABLE Table
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  The function IsGenericTableEmpty will return to the caller TRUE if
    //  the input table is empty (i.e., does not contain any elements) and
    //  FALSE otherwise.
    //

    //
    // Generic extensions for using generic structures with the avl libraries.
    //
#if (NTDDI_VERSION >= NTDDI_WINXP)
    _Must_inspect_result_
        NTSYSAPI
        BOOLEAN
        NTAPI
        RtlIsGenericTableEmptyAvl(
        _In_ PRTL_AVL_TABLE Table
        );
#endif // NTDDI_VERSION >= NTDDI_WINXP

    //
    //  As an aid to allowing existing generic table users to do (in most
    //  cases) a single-line edit to switch over to Avl table use, we
    //  have the following defines and inline routine definitions which
    //  redirect calls and types.  Note that the type override (performed
    //  by #define below) will not work in the unexpected event that someone
    //  has used a pointer or type specifier in their own #define, since
    //  #define processing is one pass and does not nest.  The __inline
    //  declarations below do not have this limitation, however.
    //
    //  To switch to using Avl tables, add the following line before your
    //  includes:
    //
    //  #define RTL_USE_AVL_TABLES 0
    //

#ifdef RTL_USE_AVL_TABLES

#undef PRTL_GENERIC_COMPARE_ROUTINE
#undef RTL_GENERIC_COMPARE_ROUTINE
#undef PRTL_GENERIC_ALLOCATE_ROUTINE
#undef RTL_GENERIC_ALLOCATE_ROUTINE
#undef PRTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_FREE_ROUTINE
#undef RTL_GENERIC_TABLE
#undef PRTL_GENERIC_TABLE

#define PRTL_GENERIC_COMPARE_ROUTINE PRTL_AVL_COMPARE_ROUTINE
#define RTL_GENERIC_COMPARE_ROUTINE RTL_AVL_COMPARE_ROUTINE
#define PRTL_GENERIC_ALLOCATE_ROUTINE PRTL_AVL_ALLOCATE_ROUTINE
#define RTL_GENERIC_ALLOCATE_ROUTINE RTL_AVL_ALLOCATE_ROUTINE
#define PRTL_GENERIC_FREE_ROUTINE PRTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_FREE_ROUTINE RTL_AVL_FREE_ROUTINE
#define RTL_GENERIC_TABLE RTL_AVL_TABLE
#define PRTL_GENERIC_TABLE PRTL_AVL_TABLE

#define RtlInitializeGenericTable               RtlInitializeGenericTableAvl
#define RtlInsertElementGenericTable            RtlInsertElementGenericTableAvl
#define RtlInsertElementGenericTableFull        RtlInsertElementGenericTableFullAvl
#define RtlDeleteElementGenericTable            RtlDeleteElementGenericTableAvl
#define RtlLookupElementGenericTable            RtlLookupElementGenericTableAvl
#define RtlLookupElementGenericTableFull        RtlLookupElementGenericTableFullAvl
#define RtlEnumerateGenericTable                RtlEnumerateGenericTableAvl
#define RtlEnumerateGenericTableWithoutSplaying RtlEnumerateGenericTableWithoutSplayingAvl
#define RtlGetElementGenericTable               RtlGetElementGenericTableAvl
#define RtlNumberGenericTableElements           RtlNumberGenericTableElementsAvl
#define RtlIsGenericTableEmpty                  RtlIsGenericTableEmptyAvl

#endif // RTL_USE_AVL_TABLES


#ifdef __cplusplus
}
#endif

#endif // __AVL_TABLE__
