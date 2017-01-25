/***************************************************************************
* sample1.cpp  -  basic select with defines and a bind variable
* -------------------
* begin     : Mon Jun 17 2002
* copyright : (C) 2002 by Marc Lavergne
***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream.h>
#include <stdlib.h>
#include <string.h>

#include "libpgci/libpgci.h"

int main4(int argc, char *argv[]) {
  PgConnection m_conn;
  if (!m_conn.connect("scott", "tiger", "SAMPLE")) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgStatement m_stmt(&m_conn);

  // -------------------------------------------------------------------------
  // first some basic DML

  m_stmt.execute("drop table dept");

  m_stmt.execute("create table dept (deptno integer, dname varchar(15), loc "
                 "varchar(15), rptdate timestamp)");

  // -------------------------------------------------------------------------
  // now define a bind variable and commit a transaction

  unsigned int v_bnd_1 = 20;
  PgPlaceholder v_bnd_1P(&v_bnd_1);

  unsigned char *v_bnd_2 = (unsigned char *)malloc(20);
  PgPlaceholder v_bnd_2P(v_bnd_2, 20);
  v_bnd_2P.setNull(true);

  //  strcpy((char*)v_bnd_2,"2001-01-01 13:01:00");

  m_stmt.parse("insert into dept (deptno, rptdate) values (:1,:2)");

  m_stmt.bind(1, &v_bnd_1P, PG_INTEGER);
  m_stmt.bind(2, &v_bnd_2P, PG_DATE);

  if (!m_stmt.execute()) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  m_conn.commit();

  // -------------------------------------------------------------------------
  // and finally define some define variables and select our data (Note: no
  // rebind reqd)

  unsigned int v_col_1;
  PgPlaceholder v_col_1P(&v_col_1);

  unsigned char *v_col_2 = (unsigned char *)malloc(20);
  PgPlaceholder v_col_2P(v_col_2, 20);

  unsigned char *v_col_3 = (unsigned char *)malloc(20);
  PgPlaceholder v_col_3P(v_col_3, 20);

  unsigned char *v_col_4 = (unsigned char *)malloc(20);
  PgPlaceholder v_col_4P(v_col_4, 20);

  m_stmt.parse("select * from dept where deptno >= :1");

  m_stmt.define(1, &v_col_1P, PG_INTEGER);
  m_stmt.define(2, &v_col_2P, PG_VARCHAR);
  m_stmt.define(3, &v_col_3P, PG_VARCHAR);
  m_stmt.define(4, &v_col_3P, PG_DATE);

  m_stmt.bind(1, &v_bnd_1P, PG_INTEGER);

  if (!m_stmt.execute()) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgCursor *m_curr = m_stmt.getCursor();

  if (m_curr == NULL) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  while (m_curr->fetch())
    printf("Result: %i %s %s\n", v_col_1, v_col_2, v_col_3);

  // -------------------------------------------------------------------------
  // cleanup and exit

  m_curr->close();

  m_conn.disconnect();

  return EXIT_SUCCESS;
}

// =========================================================================
