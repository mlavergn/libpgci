/***************************************************************************
* sample2.cpp  -  insert and select with defines and a bind variables
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

int main2(int argc, char *argv[]) {
  PgConnection m_conn;
  if (!m_conn.connect("usernameX", "passwordX", "dbX")) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgStatement m_stmt(&m_conn);

  unsigned int v_col_1;
  PgPlaceholder v_col_1P(&v_col_1);

  unsigned char *v_col_2 = (unsigned char *)malloc(20);
  PgPlaceholder v_col_2P(v_col_2, 20);

  unsigned char *v_col_3 = (unsigned char *)malloc(20);
  PgPlaceholder v_col_3P(v_col_3, 20);

  unsigned int v_bnd_1 = 50;
  PgPlaceholder v_bnd_1P(&v_bnd_1);

  unsigned char *v_bnd_2 = (unsigned char *)"IT";
  PgPlaceholder v_bnd_2P(v_bnd_2);

  unsigned char *v_bnd_3 = (unsigned char *)"ORLANDO";
  PgPlaceholder v_bnd_3P(v_bnd_3);

  // insert the data
  m_stmt.parse("insert into dept values (:1, :2, :3)");

  m_stmt.bind(1, &v_bnd_1P, PG_INTEGER);
  m_stmt.bind(2, &v_bnd_2P, PG_VARCHAR);
  m_stmt.bind(3, &v_bnd_3P, PG_VARCHAR);

  if (!m_stmt.execute()) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  m_conn.commit();

  // now select the data back
  m_stmt.parse("select * from dept");

  m_stmt.define(1, &v_col_1P, PG_INTEGER);
  m_stmt.define(2, &v_col_2P, PG_VARCHAR);
  m_stmt.define(3, &v_col_3P, PG_VARCHAR);

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

  m_curr->close();

  m_conn.disconnect();

  return EXIT_SUCCESS;
}

// =========================================================================
