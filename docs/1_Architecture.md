# Secure Storage binding for redpesk

## Architecture

The redpesk Secure Storage binding is base on the [legato.io](https://legato.io/) secure storage API.

* [secure storage](https://docs.legato.io/latest/c_secStore.html)
* [secure storage Admin](https://docs.legato.io/latest/c_secStoreAdmin.html)

The core of the secure storage is a an encrypted database.

![Architecture scheme](./img/DB_description.png)

The Secure Storage binding provides a secure API to access the encrypted database.

Every client application can store data in a key/value format in the data base, in a private section or in public section call "global".

![Architecture scheme](./img/DB_Usage_1.png)

So two client applications can only share key/value throw the "global" section.

![Architecture scheme](./img/DB_Usage_2.png)

Only an administrator can have access to the full secure storage area.

![Architecture scheme](./img/DB_Usage_3.png)

The Administration API is mostly disabled by default in Framework for security concerns.
