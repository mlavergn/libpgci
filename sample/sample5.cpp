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

int main(int argc, char *argv[]) {
  PgConnection m_conn;
  if (!m_conn.connect("scott", "tiger", "SAMPLE")) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgStatement m_stmt(&m_conn);

  // -------------------------------------------------------------------------
  // first some basic DML

  m_stmt.execute((unsigned char *)"drop table emp");

  m_stmt.execute((unsigned char *)"create table emp (empno integer, lname "
                                  "varchar(15), hiredate timestamp)");

  // -------------------------------------------------------------------------
  // now define a bind variable and commit a transaction

  PgDirectLoad m_dload(&m_conn);

  if (!m_dload.initialize((unsigned char *)"emp")) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  unsigned int v_bnd_1 = 10;
  unsigned char *v_bnd_2 = new unsigned char[15];
  unsigned char *v_bnd_3 = new unsigned char[20];

  strcpy((char *)v_bnd_2, "Hello\nWorld");
  strcpy((char *)v_bnd_3, "");

  PgPlaceholder v_bnd_P[3];

  v_bnd_P[0].init(&v_bnd_1, sizeof(int), sizeof(int));
  v_bnd_P[1].init(v_bnd_2, 15, 15);
  v_bnd_P[2].init(v_bnd_3, 20, 20);
  v_bnd_P[2].setNull(true);

  m_dload.bind(1, &v_bnd_P[0], PG_INTEGER);
  m_dload.bind(2, &v_bnd_P[1], PG_VARCHAR);
  m_dload.bind(3, &v_bnd_P[2], PG_DATE);

  // loop
  m_dload.execute();

  v_bnd_1 = 20;
  strcpy((char *)v_bnd_2, "Orlando");

  m_dload.execute();

  m_dload.complete();

  // -------------------------------------------------------------------------
  // cleanup and exit
  m_conn.disconnect();

  return EXIT_SUCCESS;
}

// =========================================================================
