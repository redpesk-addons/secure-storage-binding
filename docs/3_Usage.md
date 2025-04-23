
# Usage

To test the service you can run it on your host.

## Start the service

At first you need to start the service on your host.

```bash
cd service-secure-storage
cd build
export AFB_PASSWD_DIR=$(pwd)/package-test/var
mkdir -p ${AFB_PASSWD_DIR}
echo "test_encryption_key" > ${AFB_PASSWD_DIR}/test.passwd
```

To launch the binding with the `afb-binder`, execute the following command:

```bash
$afb-binder -b ./secstorage/afb-service-secure-storage.so -vvv
``` 



### Start secstorage API client

```bash
$afb-client -H localhost:1234/api

$secstorage Write {"key":"name","value":"Iheb"}
ON-REPLY 1:secstorage/Write: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}


$secstorage Read {"key":"name"}
ON-REPLY 2:secstorage/Read: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "value":"Iheb"
  }
}


$secstorage Delete {"key":"name"}
ON-REPLY 3:secstorage/Delete: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}
```

### Start secstorage API secstoreglobal

```bash
$afb-client -H localhost:1234/api

$secstoreglobal Write {"key":"company","value":"IoT.bzh"}
ON-REPLY 5:secstoreglobal/Write: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}


$secstoreglobal Read {"key":"company"}
ON-REPLY 6:secstoreglobal/Read: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "value":"IoT.bzh"
  }
}


$secstoreglobal Delete {"key":"company"}
ON-REPLY 7:secstoreglobal/Delete: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}
```

### Start secstorage API secstoreadmin

```bash
$afb-client -H localhost:1234/api

$ secstoreadmin Write {"key":"/NoLabel/email","value":"iheb.bengaraali@iot.bzh"}
ON-REPLY 8:secstoreadmin/Write: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}

$ secstoreadmin Read {"key":"/NoLabel/email"}
ON-REPLY 9:secstoreadmin/Read: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "value":"iheb.bengaraali@iot.bzh"
  }
}

$ secstoreadmin Delete {"key":"/NoLabel/email"}
ON-REPLY 10:secstoreadmin/Delete: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}

$ secstoreadmin GetTotalSpace
ON-REPLY 11:secstoreadmin/GetTotalSpace: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "totalSize":8192,
    "freeSize":16769024
  }
}


$ secstoreasecstoreadmindmin CreateIter {"key":"/NoLabel/"}
ON-REPLY 13:secstoreadmin/CreateIter: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "iterator":1
  }
}


$ secstoreadmin Next
ON-REPLY 14:secstoreadmin/Next: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}


$ secstoreadmin GetEntry
ON-REPLY 16:secstoreadmin/GetEntry: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  },
  "response":{
    "value":"/global/name"
  }
}


$ secstoreadmin DeleteIter
ON-REPLY 17:secstoreadmin/DeleteIter: OK
{
  "jtype":"afb-reply",
  "request":{
    "status":"success",
    "code":0
  }
}
```
