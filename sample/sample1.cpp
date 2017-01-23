/**
* sample1.cpp  -  basic select with defines and a bind variable
* -------------------
* begin     : Mon Jun 17 2002
* copyright : (C) 2002 by Marc Lavergne
*/

#ifdef HAVE_CONFIG_H
#include <config>
#endif

#include <iostream>
#include <string>
#include <stdlib.h>

#include <libpgci.h>

int main(int argc, char *argv[])
{
  PgConnection m_conn;
  if (!m_conn.connect("usernameX","passwordX","dbX"))
  {
    printf("Connect Error:\n\t%s\n",m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgStatement m_stmt(&m_conn);
  m_stmt.parse("select * from dept where deptno >= :1");

  unsigned int   v_col_1;
  PgPlaceholder  v_col_1P(&v_col_1);

  unsigned char* v_col_2 = (unsigned char*)malloc(20);
  PgPlaceholder  v_col_2P(v_col_2,20);

  unsigned char* v_col_3 = (unsigned char*)malloc(20);
  PgPlaceholder  v_col_3P(v_col_3,20);

  unsigned int   v_bnd_1 = 20;
  PgPlaceholder  v_bnd_1P(&v_bnd_1);

  m_stmt.define(1,&v_col_1P,PG_INTEGER);
  m_stmt.define(2,&v_col_2P,PG_VARCHAR);
  m_stmt.define(3,&v_col_3P,PG_VARCHAR);

  m_stmt.bind(1,&v_bnd_1P,PG_INTEGER);

  if (!m_stmt.execute())
  {
    fprintf(stderr,"Execute Error:\n\t%s\n",m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgCursor* m_curr = m_stmt.getCursor();

  if (m_curr == NULL)
  {
    printf("Connect Error:\n\t%s\n",m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  while (m_curr->fetch())
    printf("Result: %i %s %s\n",v_col_1,v_col_2,v_col_3);

  m_curr->close();

  m_conn.disconnect();

  return EXIT_SUCCESS;
}
