# libpgci
--
### PostgreSQL OCI like API in C++

Introduction
--
This is a piece of code I wrote in a former life that I found and decided to see if it still worked. It did, so I'm putting it up on GitHub for posterity. I haven't looked at it since 2002, so take that for what it's worth.

It was unlicensed at the time, so I'm re-releasing it under an MIT license.

I made a few small changes to get it to build on MacOS and under the LLVM toolchain. Other than that it's all original. 

Dependencies
--

* [PostgreSQL](https://www.postgresql.org)

Installation
--
```bash
	make install
```

Examples
--
```c++

PgConnection conn;
if (!conn.connect("scott","tiger","db")) {
	return EXIT_FAILURE;
}

PgStatement stmt(&conn);
stmt.parse("select name from dept where deptno >= :1");

unsigned int   bnd1 = 20;
PgPlaceholder  bnd1Ph(&bnd1);

stmt.bind(1, &bnd1Ph,PG_INTEGER);

if (!stmt.execute()) {
	return EXIT_FAILURE;
}

PgCursor* curr = stmt.getCursor();

while (curr->fetch()) {
	printf("Result: %s\n", col);
}

curr->close();

conn.disconnect();
```
