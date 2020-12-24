
# Usage

To test the service you can run it on your host.

## Start the service

At first you need to start the service on your host.

```bash
cd service-secure-storage
cd build;
export AFB_PASSWD_DIR=$(pwd)/package-test/var
mkdir -p ${AFB_PASSWD_DIR}
echo "test_encryption_key" > ${AFB_PASSWD_DIR}/test.passwd

afb-daemon --port=1234 \
           --no-ldpaths \
           --binding=./package/lib/afb-service-secure-storage.so\
           --workdir=. \
           --roothttp=../htdocs \
           --token= \
           --verbose \
           --ws-server=unix:/tmp/secstorage \
           --ws-server=unix:/tmp/secstoreglobal \
           --ws-server=unix:/tmp/secstoreadmin
```

### Start secstorage API client

```bash
$afb-client-demo -d unix:/tmp/secstorage

$ Write {"key":"name","value":"Ronan"}
{"jtype":"afb-reply","request":{"status":"success"}}

$ Read {"key":"name"}
{"jtype":"afb-reply","request":{"status":"success","info":"db success: read /NoLabel/name=Ronan."},"response":{"value":"Ronan"}}

$ Delete {"key":"name"}
{"jtype":"afb-reply","request":{"status":"success"}}
```

### Start secstorage API secstoreglobal

```bash
$ afb-client-demo -d unix:/tmp/secstoreglobal

$ Write {"key":"company","value":"IoT.bzh"}
{"jtype":"afb-reply","request":{"status":"success"}}

$ Read {"key":"company"}
{"jtype":"afb-reply","request":{"status":"success","info":"db success: read /global/company=IoT.bzh."},"response":{"value":"IoT.bzh"}}

$ Delete {"key":"company"}
{"jtype":"afb-reply","request":{"status":"success"}}
```

### Start secstorage API secstoreadmin

```bash
$ afb-client-demo -d unix:/tmp/secstoreadmin

$ Write {"key":"/NoLabel/email","value":"ronan.lemartret@iot.bzh"}
{"jtype":"afb-reply","request":{"status":"success"}}

$ Read {"key":"/NoLabel/email"}
{"jtype":"afb-reply","request":{"status":"success","info":"db success: read /NoLabel/email=ronan.lemartret@iot.bzh."},"response":{"value":"ronan.lemartret@iot.bzh"}}

$ Delete {"key":"/NoLabel/email"}

$ GetTotalSpace
{"jtype":"afb-reply","request":{"status":"success","info":"DB totalSize is 8192 freeSize 16769024"},"response":{"totalSize":8192,"freeSize":16769024}}

$ createiter {"key":"/NoLabel/"}
{"jtype":"afb-reply","request":{"status":"success"},"response":{"iterator":1}}

$ next
{"jtype":"afb-reply","request":{"status":"success"}}

$ getentry
{"jtype":"afb-reply","request":{"status":"success"},"response":{"value":"/NoLabel/email"}}

$ deleteIter

$ createiter {"key":"/NoLabel/error"}

$ GetSize {"key":"/NoLabel/"}
{"jtype":"afb-reply","request":{"status":"success","info":"DB gettotalspace of /NoLabel/ is 384"},"response":{"size":384}}

$ copymetato {"path":"/tmp/test_copy.db"}
{"jtype":"afb-reply","request":{"status":"success"}
```
