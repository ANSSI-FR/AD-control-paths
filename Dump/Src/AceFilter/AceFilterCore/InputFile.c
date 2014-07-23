/* -- INCLUDES ---------- */
#include "InputFile.h"


/* -- VARIABLES ---------- */
/* -- FUNCTIONS ---------- */
DWORD ParseLine (
   _In_  LPTSTR   szLine,
   _In_  DWORD    dwMaxLineSize,
   _In_  DWORD    dwTokenCount,
   _Inout_ LPTSTR   pszTokens[]
) {
   DWORD dwLastTokenPos = 0;
   DWORD dwLinePos = 0;
   DWORD dwAssigned = 0;

   while( (szLine[dwLinePos] != '\0' ) && (dwAssigned < dwTokenCount) && (dwLinePos < dwMaxLineSize) ) {
      if( szLine[dwLinePos] == '\t' ) {
         pszTokens[ dwAssigned++ ] = szLine + dwLastTokenPos;
         szLine[ dwLinePos++ ] = '\0';
         dwLastTokenPos = dwLinePos;
      } else {
         dwLinePos++;
      }
   }

   // Last element
   pszTokens[ dwAssigned++ ] = szLine + dwLastTokenPos;

   return dwAssigned;
}


BOOL ReadLine (
   _In_                          PBYTE    *pInput,
   _In_                          LONGLONG llSizeInput,
   _Inout_                         LONGLONG *llPositionInput, 
   _Out_writes_(dwMaxLineSize)   PTCHAR   pOutputLine,
   _In_                          DWORD    dwMaxLineSize
   )
{
   // TODO : unicode

   DWORD dwSizeToCopy = 0;

   // Primary verifications
   if( *llPositionInput >= llSizeInput || **pInput == '\0' ) {
      return FALSE;
   }

   // Computing the effective size to copy
   while( dwSizeToCopy < dwMaxLineSize-1 && **pInput != '\r' && **pInput != '\n' && **pInput != '\0' ) {
      (*pInput)++;
      (*llPositionInput)++;
      dwSizeToCopy++;
   }

   // Copying
   RtlCopyMemory(pOutputLine, *pInput - dwSizeToCopy, dwSizeToCopy);
   pOutputLine[dwSizeToCopy] = '\0';

   // Positionning the input to the beginning of the next line
   while( (**pInput == '\r' || **pInput == '\n') && **pInput != '\0' && *llPositionInput < llSizeInput ) {
      (*pInput)++;
      (*llPositionInput)++;
   }

   return TRUE;
}

// C6262 : Excessive stack usage : Function uses '131128' bytes of stack : exceeds / analyze : stacksize '16384'.Consider moving some data to heap.
// not so important
#pragma warning(suppress: 6262)

DWORD ReadParseTsvLine(
    _In_                          PBYTE    *pInput,
    _In_                          LONGLONG llSizeInput,
    _Inout_                         LONGLONG *llPositionInput,
    _In_  DWORD    dwTokenCount,
    _Inout_ LPTSTR   pszTokens[],
    _In_ LPTSTR skipHeaderFirstToken
    ) {
    BOOL bResult = FALSE, bSkipLine = FALSE;
    TCHAR line[MAX_SIZE_LINE];
    DWORD tokenAssigned = 0;

    do {
        bResult = ReadLine(pInput, llSizeInput, llPositionInput, line, MAX_SIZE_LINE);
        if (!bResult) {
            return FALSE;
        }

        tokenAssigned = ParseLine(line, MAX_SIZE_LINE, dwTokenCount, pszTokens);
        if (tokenAssigned != dwTokenCount) {
            FATAL(_T("Wrong number of token : <%u/%u>"), tokenAssigned, dwTokenCount); // TODO : more information would be nice here (line num, file name, etc.)
        }

        bSkipLine = STR_EQ(skipHeaderFirstToken, pszTokens[0]);
        if (bSkipLine) {
            LOG(Dbg, _T("Found a header, skipping"));
        }
    } while (bSkipLine);

    return TRUE;
}

// C6262 : Excessive stack usage : Function uses '131144' bytes of stack : exceeds / analyze : stacksize '16384'.Consider moving some data to heap
// not so important.
#pragma warning(suppress: 6262)

void ForeachLine (
   _In_     PINPUT_FILE       pInputFile,
   _In_     DWORD             dwTokenCount,
   _In_     FN_LINE_CALLBACK  pfnCallback,
   _Inout_  PVOID             pParam
   ) {
      PVOID pvMappingBackup = pInputFile->pvMapping;
      LONGLONG llPosition = 0;
      TCHAR chLine[MAX_SIZE_LINE];
      LPTSTR *pszTokens = NULL;
      DWORD dwTokenAssigned = 0;
      DWORD dwLineNumber = 0;
      DWORD dwNextProgressStep = INPUT_FILE_PERCENT_PROGRESS_DELTA;
      ULONGLONG ullStartTime = GetTickCount64();

      pszTokens = (LPTSTR*)LocalAllocCheckX( sizeof(LPTSTR) * dwTokenCount );

      LOG(Info, _T("Reading lines of file <%s>"), pInputFile->szName);

      //
      // Skip the first line (headers)
      //
      ReadLine((PBYTE*)&pvMappingBackup, pInputFile->fileSize.QuadPart, &llPosition, chLine, MAX_SIZE_LINE);

      //
      // Loop : read and parse line, and transfer parsed tokens to the callback
      //
      while( ReadLine((PBYTE*)&pvMappingBackup, pInputFile->fileSize.QuadPart, &llPosition, chLine, MAX_SIZE_LINE) ) {
         dwLineNumber++;
         dwTokenAssigned = ParseLine(chLine, MAX_SIZE_LINE, dwTokenCount, pszTokens);

         if( dwTokenAssigned != dwTokenCount ) {
            LOG(Err, _T("Wrong number of token for file <%s> on line <%u> : <%u/%u>"), pInputFile->szName, dwLineNumber, dwTokenAssigned, dwTokenCount );
         } else {

            ////DBG
            //{
            //   DWORD i;
            //   LOG(Dbg, _T("Line <%u> :"), dwLineNumber);
            //   for( i=0; i<dwTokenAssigned; i++ ) {
            //      LOG(Dbg, SUB_LOG(_T("<%u/%u> : <%s>")), i+1, dwTokenAssigned, pszTokens[i]);
            //   }
            //}
            ////DBG

            pfnCallback(dwLineNumber, pszTokens, dwTokenAssigned, pParam);
         }

         if( ((100 * ((PBYTE)pvMappingBackup - (PBYTE)pInputFile->pvMapping)) / pInputFile->fileSize.QuadPart) > dwNextProgressStep ) {
            LOG(Info, SUB_LOG(_T("<%03u%%> <%.3fs>")), dwNextProgressStep, (GetTickCount64() - ullStartTime) / 1000.0 );
            dwNextProgressStep += INPUT_FILE_PERCENT_PROGRESS_DELTA;
         }
      }

      LOG(Info, SUB_LOG(_T("<%03u%%> <%.3fs>")), dwNextProgressStep, (GetTickCount64() - ullStartTime) / 1000.0 );
      LOG(Info, _T("processed <%u> lines for file <%s>"), dwLineNumber, pInputFile->szName);

      LocalFreeCheckX(pszTokens);
}

BOOL InitInputFile (
   _In_     LPTSTR      szPath,
   _In_     LPTSTR      szName,
   _Inout_  PINPUT_FILE pInputFile
   ) {
      BOOL bResult;

      ZeroMemory(pInputFile, sizeof(INPUT_FILE));
      pInputFile->szPath = szPath;
      pInputFile->szName = szName;

      pInputFile->hFile = CreateFile(pInputFile->szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if( pInputFile->hFile == INVALID_HANDLE_VALUE ) {
         LOG(Err, _T("unable to open <%s> input file : <%u>"), pInputFile->szName, GetLastError());
         goto error;
      }

      bResult = GetFileSizeEx(pInputFile->hFile, &pInputFile->fileSize);
      if( !bResult ) {
         LOG(Err, _T("unable to get <%s> input file size : <%u>"), pInputFile->szName, GetLastError());
         goto error;
      }

      pInputFile->hMapping = CreateFileMapping(pInputFile->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
      if( !pInputFile->hMapping ) {
         LOG(Err, _T("unable to create <%s> input file mapping : <%u>"), pInputFile->szName, GetLastError());
         goto error;
      }

      pInputFile->pvMapping = MapViewOfFile(pInputFile->hMapping, FILE_MAP_READ, 0, 0, 0);
      if( !pInputFile->pvMapping ) {
         LOG(Err, _T("unable to map view of <%s> input file : <%u>"), pInputFile->szName, GetLastError());
         goto error;
      }

      return TRUE;

error:
      CloseInputFile(pInputFile);
      return FALSE;
}

void ResetInputFile(
    _In_ PINPUT_FILE inputFile,
    _Inout_ PVOID *mapping,
    _Inout_ PLONGLONG position
    ) {
    *mapping = inputFile->pvMapping;
    *position = 0;
}

void CloseInputFile (
   _Inout_  PINPUT_FILE pInputFile
   ) {
      if( pInputFile->hFile != INVALID_HANDLE_VALUE ) {
         if( pInputFile->hMapping ) {
            if( pInputFile->pvMapping) {
               UnmapViewOfFile( pInputFile->pvMapping );
            }
            CloseHandle( pInputFile->hMapping );
         }
         CloseHandle( pInputFile->hFile );
      }
}
