# Secure Storage

The Redpesk Secure Storage binding API is base on the [legato.io](https://legato.io/) secure storage API:

* [le_secStore.api](https://github.com/legatoproject/legato-af/blob/master/interfaces/le_secStore.api): For the common application case.
* [secStoreAdmin.api](https://github.com/legatoproject/legato-af/blob/master/interfaces/secureStorage/secStoreAdmin.api): For Administration of the secure storage.

The API and API documentation should be close to API Legato as possible.

## Secure Storage API "secstorage"

Secure storage can be used to store sensitive information like passwords, keys, certificates, etc.
All data in the secure storage is in an encrypted format.
Each application using this API only has access to its own secure storage data.

| Verb          | Description                                       |
|---------------|---------------------------------------------------|
|Write          | Writes an item to secure storage. |
|Read           | Reads an item from secure storage. |
|Delete         | Deletes an item from secure storage. |

## Secure Storage Global API "secstoreglobal"

Same as the "secstorage" API but each application using this "secstoreglobal" API can share access to secure storage data.

| Verb          | Description                                       |
|---------------|---------------------------------------------------|
|Write          | Writes an item to secure storage. |
|Read           | Reads an item from secure storage. |
|Delete         | Deletes an item from secure storage. |

## Secure Storage Administration API "secstoreadmin"

The full Administration API should only be used by privileged users.
This API is mostly disabled by default in Framework for security concerns. You can activate the Administration API if you compile the code with ALLOW_SECS_ADMIN.

| Verb          | Description                                       |
|---------------|---------------------------------------------------|
|CreateIter     | Create an iterator for listing entries in secure storage under the specified path. |
|DeleteIter     | Deletes an iterator. |
|Next           | Go to the next entry in the iterator. |
|GetEntry       | Get the current entry's name. |
|Write          | Writes a buffer of data into the specified path in secure storage |
|Read           | Reads an item from secure storage. |
|CopyMetaTo     | Copy the meta file to the specified path. |
|Delete         | Recursively deletes all items under the specified path and the specified path from secure storage.|
|GetSize        | Gets the size, in bytes, of all items under the specified path. |
|GetTotalSpace  | Gets the total space and the available free space in secure storage. |

If you compile the code without ALLOW_SECS_ADMIN, the "GetSize" and "GetTotalSpace" verb are still available.
