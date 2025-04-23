from afb_test import AFBTestCase, configure_afb_binding_tests, run_afb_binding_tests

import libafb

import pdb;

bindings = {"secstorage": f"secure-storage-binding.so"}


def setUpModule():
    configure_afb_binding_tests(bindings=bindings)


class TestSecureStorage(AFBTestCase):
    
    def test_write_read_delete(self):
        """Test write, read and delete"""

        # Write key-value
        r = libafb.callsync(self.binder, "secstorage", "Write", {"key": "name", "value": "iheb"})
        assert r.status == 0

        # Read value back
        r = libafb.callsync(self.binder, "secstorage", "Read", {"key": "name"})
        assert r.status == 0
        assert r.args[0]['value'] == "iheb"

        # Delete the key
        r = libafb.callsync(self.binder, "secstorage", "Delete", {"key": "name"})
        assert r.status == 0


    def test_write_read_delete_global(self):
        """Test write, read and delete global"""

        # Write key-value
        r = libafb.callsync(self.binder, "secstoreglobal", "Write", {"key":"company","value":"IoT.bzh"})
        assert r.status == 0

        # Read value back
        r = libafb.callsync(self.binder, "secstoreglobal", "Read", {"key": "company"})
        assert r.status == 0
        assert r.args[0]['value'] == "IoT.bzh"

        # Delete the key
        r = libafb.callsync(self.binder, "secstoreglobal", "Delete", {"key": "company"})
        assert r.status == 0


    def test_read_delete_fail(self):
        """Test read and delete failure"""

        try:
            # Read non-existent value
            r = libafb.callsync(self.binder, "secstorage", "Read", {"key": "key-not-exist"})
            assert False
        except RuntimeError as e:
            # Verify that the exception matches the expected one
            assert str(e) == "invalid-request", f"Unexpected error message: {str(e)}"

        try:
            # Delete non-existent value
            r = libafb.callsync(self.binder, "secstorage", "Delete", {"key": "key-not-exist"})
            assert False
        except RuntimeError as e:
            assert str(e) == "invalid-request", f"Unexpected error message: {str(e)}"
            
    def test_write_read_delete_admin(self):
        """Test write, read, delete and iterator features of secstoreadmin"""

        key = "/NoLabel/email"
        value = "iheb.bengaraali@iot.bzh"

        # Write key-value
        r = libafb.callsync(self.binder, "secstoreadmin", "Write", {"key": key, "value": value})
        assert r.status == 0

        # Read value back
        r = libafb.callsync(self.binder, "secstoreadmin", "Read", {"key": key})
        assert r.status == 0
        assert r.args[0]['value'] == value

        # Get storage space
        r = libafb.callsync(self.binder, "secstoreadmin", "GetTotalSpace", None)
        assert r.status == 0
        assert r.args[0]['totalSize'] >= 512

        # Create iterator for the namespace
        r = libafb.callsync(self.binder, "secstoreadmin", "CreateIter", {"key": "/NoLabel/"})
        assert r.status == 0

        # Move to next entry
        r = libafb.callsync(self.binder, "secstoreadmin", "Next", None)
        assert r.status == 0

        # Get entry name
        r = libafb.callsync(self.binder, "secstoreadmin", "GetEntry", None)
        assert r.status == 0

        # Delete iterator
        r = libafb.callsync(self.binder, "secstoreadmin", "DeleteIter", None)
        assert r.status == 0

        # Delete the key
        r = libafb.callsync(self.binder, "secstoreadmin", "Delete", {"key": key})
        assert r.status == 0


if __name__ == "__main__":
    run_afb_binding_tests(bindings)