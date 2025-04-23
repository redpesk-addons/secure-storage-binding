SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Install or reinstall dependencies
for p in afb-binding-devel afb-libpython afb-test-py afb-cmake-modules libdb-devel; do
    sudo rpm -q $p && sudo dnf update -y $p || sudo dnf install -y $p 
done

# Install missing tools if needed
for p in g++ lcov; do
    command -v $p 2>/dev/null 2>&1 || sudo dnf install -y $p
done

CMAKE_COVERAGE_OPTIONS="-DCMAKE_C_FLAGS=--coverage -DCMAKE_CXX_FLAGS=--coverage"

rm -rf build
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=COVERAGE -DSECSTOREADMIN=ON ..
make
export AFB_PASSWD_DIR=$(pwd)
mkdir -p ${AFB_PASSWD_DIR}
echo "test_encryption_key" > ${AFB_PASSWD_DIR}/test.passwd
LD_LIBRARY_PATH=. python ../test/py-test.py -vvv



# #
# # Coverage
# #
# rm -f lcov_cobertura.py
# wget https://raw.github.com/eriwen/lcov-to-cobertura-xml/master/lcov_cobertura/lcov_cobertura.py

# rm app.info
# lcov --directory . --capture --output-file app.info
# # remove system headers from coverage
# lcov --remove app.info '/usr/*' -o app_filtered.info
# # output summary (for Gitlab CI coverage summary)
# lcov --list app_filtered.info
# # generate a report (for source annotation in MR)
# python lcov_cobertura.py app_filtered.info -o ./coverage.xml

# genhtml -o html app_filtered.info

# afb-client -H localhost:1234/api sec
