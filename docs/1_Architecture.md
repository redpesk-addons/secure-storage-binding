# Secure Storage binding for Redpesk®

## Architecture

The Redpesk Secure Storage binding is base on the [legato.io](https://legato.io/) secure storage API.

* [secure storage](https://docs.legato.io/latest/c_secStore.html)
* [secure storage Admin](https://docs.legato.io/latest/c_secStoreAdmin.html)

The core of the secure storage is a an encrypt database.
____________________________________________
![Architecture scheme](./img/DB_description.png)
____________________________________________

The Secure Storage binding provides secure API to access the encrypt database.

Every client application can store data in a key/value format in the data base, in a private section or in public section call "global".
____________________________________________
![Architecture scheme](./img/DB_Usage_1.png)
____________________________________________

So two client application can only share key/value throw the "global" section.
____________________________________________
![Architecture scheme](./img/DB_Usage_2.png)
____________________________________________

Only a administrator can have access to the full secure storage area.
____________________________________________
![Architecture scheme](./img/DB_Usage_3.png)
____________________________________________
The Administration API is mostly disabled by default in Framework for security concerns.
