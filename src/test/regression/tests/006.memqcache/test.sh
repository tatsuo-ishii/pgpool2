#!/usr/bin/env bash
#-------------------------------------------------------------------
# test script for memqcache.
# requires Java PostgreSQL JDBC driver.
PGBENCH=$PGBENCH_PATH

WHOAMI=`whoami`
source $TESTLIBS
TESTDIR=testdir

for mode in s r n
do
	rm -fr $TESTDIR
	mkdir $TESTDIR
	cd $TESTDIR

# create test environment
	echo -n "creating test environment..."
	$PGPOOL_SETUP -m $mode -n 2 || exit 1
	echo "done."

	echo "memory_cache_enabled = on" >> etc/pgpool.conf
	echo "white_memqcache_table_list = 'white_v'" >> etc/pgpool.conf
	echo "black_memqcache_table_list = 'black_t'" >> etc/pgpool.conf

	source ./bashrc.ports

	export PGPORT=$PGPOOL_PORT

	echo "jdbc.url=jdbc:postgresql://localhost:$PGPOOL_PORT/test" > jdbctest.prop
	echo "jdbc.user=$WHOAMI" >> jdbctest.prop
	echo "jdbc.password=" >> jdbctest.prop
	cp ../jdbctest.java .
	javac jdbctest.java
	export CLASSPATH=.:$JDBC_DRIVER
	./startall
	wait_for_pgpool_startup

	psql test <<EOF
CREATE TABLE t1 (i int);
CREATE TABLE black_t (i int);
CREATE VIEW normal_v AS SELECT * FROM t1;
CREATE VIEW white_v AS SELECT * FROM t1;

SELECT * FROM t1;
SELECT * FROM t1;
SELECT * FROM black_t;
SELECT * FROM black_t;
SELECT * FROM normal_v;
SELECT * FROM normal_v;
SELECT * FROM white_v;
SELECT * FROM white_v;
EOF

	grep "fetched from cache" log/pgpool.log | grep t1 > /dev/null || exit 1
	grep "fetched from cache" log/pgpool.log | grep black_t > /dev/null && exit 1
	grep "fetched from cache" log/pgpool.log | grep normal_v > /dev/null && exit 1
	grep "fetched from cache" log/pgpool.log | grep white_v > /dev/null || exit 1

	java jdbctest > result.txt 2>&1
	cmp ../expected.txt result.txt
	if [ $? != 0 ];then
		./shutdownall
		exit 1
	fi

	./shutdownall

	cd ..
done

exit 0
