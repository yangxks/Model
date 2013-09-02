#!/bin/sh


PLATFORM=`uname -i`
SVNVERSION=""

if [ -e .svn -a -e /usr/bin/svn ]; then
	LANG="C" SVNVERSION=`svn info | grep Revision | awk '{print $2}'`
fi

installfile="install_test_tools_"$PLATFORM"_"$SVNVERSION".bin"

make clean;
CFLAGS="-O2 -Wall -Werror" make;
strip client client1 ab server;

mkdir .install/
cp -af client client1 ab server .install/
tar zcf .install.tar.gz .install/


cat > $installfile << EOF
#!/bin/sh

rm -fr /usr/bin/testclient;
rm -fr /usr/bin/testclient1;
rm -fr /usr/bin/testclient2;
rm -fr /usr/bin/ab;
rm -fr /usr/bin/testserver;

sed -n '0,/^0011-ANHK-BLANK$/!p'  \$0 > .install.tar.gz;

tar zxf .install.tar.gz;
cp -af .install/client /usr/bin/testclient;
cp -af .install/client1 /usr/bin/testclient1;
cp -af .install/ab /usr/bin/ab;
cp -af .install/server /usr/bin/testserver;

rm -fr .install .install.tar.gz

echo
echo "Program  --> MaxConnections : testclient"
echo "测试程序 --> 最大连接数     : testclient"
echo
echo "Program  --> NewConnections : testclient1"
echo "测试程序 --> 新建连接数     : testclient1"
echo
echo "Program  --> MaxRequests    : ab"
echo "测试程序 --> HTTP请求数     : ab"
echo

exit;
0011-ANHK-BLANK
EOF

cat .install.tar.gz >> $installfile

rm -fr .install/ .install.tar.gz

md5sum $installfile > $installfile.md5
