// AUTO-GENERATED FILE DO NOT EDIT
// See src/mongo/base/generate_error_codes.py
/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <string>

#include "mongo/base/string_data.h"
#include "mongo/client/export_macros.h"

namespace mongo {

    /**
     * This is a generated class containing a table of error codes and their corresponding error
     * strings. The class is derived from the definitions in src/mongo/base/error_codes.err file.
     *
     * Do not update this file directly. Update src/mongo/base/error_codes.err instead.
     */

    class MONGO_CLIENT_API ErrorCodes {
    public:
        enum Error {
            OK = 0,
            InternalError = 1,
            BadValue = 2,
            OBSOLETE_DuplicateKey = 3,
            NoSuchKey = 4,
            GraphContainsCycle = 5,
            HostUnreachable = 6,
            HostNotFound = 7,
            UnknownError = 8,
            FailedToParse = 9,
            CannotMutateObject = 10,
            UserNotFound = 11,
            UnsupportedFormat = 12,
            Unauthorized = 13,
            TypeMismatch = 14,
            Overflow = 15,
            InvalidLength = 16,
            ProtocolError = 17,
            AuthenticationFailed = 18,
            CannotReuseObject = 19,
            IllegalOperation = 20,
            EmptyArrayOperation = 21,
            InvalidBSON = 22,
            AlreadyInitialized = 23,
            LockTimeout = 24,
            RemoteValidationError = 25,
            NamespaceNotFound = 26,
            IndexNotFound = 27,
            PathNotViable = 28,
            NonExistentPath = 29,
            InvalidPath = 30,
            RoleNotFound = 31,
            RolesNotRelated = 32,
            PrivilegeNotFound = 33,
            CannotBackfillArray = 34,
            UserModificationFailed = 35,
            RemoteChangeDetected = 36,
            FileRenameFailed = 37,
            FileNotOpen = 38,
            FileStreamFailed = 39,
            ConflictingUpdateOperators = 40,
            FileAlreadyOpen = 41,
            LogWriteFailed = 42,
            CursorNotFound = 43,
            UserDataInconsistent = 45,
            LockBusy = 46,
            NoMatchingDocument = 47,
            NamespaceExists = 48,
            InvalidRoleModification = 49,
            ExceededTimeLimit = 50,
            ManualInterventionRequired = 51,
            DollarPrefixedFieldName = 52,
            InvalidIdField = 53,
            NotSingleValueField = 54,
            InvalidDBRef = 55,
            EmptyFieldName = 56,
            DottedFieldName = 57,
            RoleModificationFailed = 58,
            CommandNotFound = 59,
            DatabaseNotFound = 60,
            ShardKeyNotFound = 61,
            OplogOperationUnsupported = 62,
            StaleShardVersion = 63,
            WriteConcernFailed = 64,
            MultipleErrorsOccurred = 65,
            ImmutableField = 66,
            CannotCreateIndex = 67,
            IndexAlreadyExists = 68,
            AuthSchemaIncompatible = 69,
            ShardNotFound = 70,
            ReplicaSetNotFound = 71,
            InvalidOptions = 72,
            InvalidNamespace = 73,
            NodeNotFound = 74,
            WriteConcernLegacyOK = 75,
            NoReplicationEnabled = 76,
            OperationIncomplete = 77,
            CommandResultSchemaViolation = 78,
            UnknownReplWriteConcern = 79,
            RoleDataInconsistent = 80,
            NoClientContext = 81,
            NoProgressMade = 82,
            RemoteResultsUnavailable = 83,
            IndexOptionsConflict = 85,
            IndexKeySpecsConflict = 86,
            NetworkTimeout = 89,
            OutdatedClient = 101,
            IncompatibleAuditMetadata = 102,
            NotMaster = 10107,
            DuplicateKey = 11000,
            InterruptedAtShutdown = 11600,
            Interrupted = 11601,
            BackgroundOperationInProgressForDatabase = 12586,
            BackgroundOperationInProgressForNamespace = 12587,
            OutOfDiskSpace = 14031,
            MaxError
        };

        static std::string errorString(Error err);

        /**
         * Parses an Error from its "name".  Returns UnknownError if "name" is unrecognized.
         *
         * NOTE: Also returns UnknownError for the string "UnknownError".
         */
        static Error fromString(const StringData& name);

        /**
         * Casts an integer "code" to an Error.  Unrecognized codes are preserved, meaning
         * that the result of a call to fromInt() may not be one of the values in the
         * Error enumeration.
         */
        static Error fromInt(int code);

        static bool isNetworkError(Error err);
        static bool isInterruption(Error err);
        static bool isIndexCreationError(Error err);
    };

}  // namespace mongo
