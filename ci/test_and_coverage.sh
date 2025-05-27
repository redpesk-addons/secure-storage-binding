SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Install or reinstall dependencies
for p in afb-binding-devel afb-libpython afb-test-py afb-cmake-modules libdb-devel afb-helpers4-devel; do
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
cmake -DSECSTOREADMIN=ON ${CMAKE_COVERAGE_OPTIONS} ..
make
export AFB_PASSWD_DIR=$(pwd)
mkdir -p ${AFB_PASSWD_DIR}
echo "test_encryption_key" > ${AFB_PASSWD_DIR}/test.passwd
LD_LIBRARY_PATH=. python ../test/py-test.py -vvv




#
# Coverage
#
cd ..
rm -f lcov_cobertura*
wget https://github.com/eriwen/lcov-to-cobertura-xml/releases/download/v2.1.1/lcov_cobertura-2.1.1.tar.gz
tar xzvf lcov_cobertura*tar.gz

rm app.info
lcov --directory . --capture --output-file app.info
# remove system headers from coverage
lcov --remove app.info '/usr/*' -o app_filtered.info
# output summary (for Gitlab CI coverage summary)
lcov --list app_filtered.info
# generate a report (for source annotation in MR)
PYTHONPATH=./lcov_cobertura-2.1.1/ python -m lcov_cobertura app_filtered.info -o ./coverage.xml

genhtml -o html app_filtered.info

