/***************************************************************************
* sample3.cpp  -  insert and select btyea binary data
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

#include <sys/stat.h>

#include "libpgci/libpgci.h"

int main3(int argc, char *argv[]) {
  struct stat v_inf;

  // open the file for reading in binary mode
  char *v_file_in = "/tmp/snapshot.jpg";
  FILE *v_bin = fopen(v_file_in, "rb");
  stat(v_file_in, &v_inf);
  int v_size = v_inf.st_size;
  unsigned char *v_buf = (unsigned char *)alloca(v_size);
  int v_bytes_in = fread(v_buf, 1, v_size, v_bin);
  printf("Read %i bytes of %i bytes from '%s'\n", v_bytes_in, v_size,
         v_file_in);
  fclose(v_bin);

  PgConnection m_conn;
  if (!m_conn.connect("usernameX", "passwordX", "dbX")) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgStatement m_stmt(&m_conn);

  unsigned int v_col_1;
  PgPlaceholder v_col_1P(&v_col_1);

  unsigned char *v_col_2 = (unsigned char *)malloc(v_size);
  PgPlaceholder v_col_2P(v_col_2, v_size);

  unsigned int v_bnd_1 = 10;
  PgPlaceholder v_bnd_1P(&v_bnd_1);

  unsigned char *v_bnd_2 = v_buf;
  PgPlaceholder v_bnd_2P(v_bnd_2, v_size);

  // insert the data
  m_stmt.parse("insert into bt (c1, c2) values (:1, :2)");

  m_stmt.bind(1, &v_bnd_1P, PG_INTEGER);
  m_stmt.bind(2, &v_bnd_2P, PG_BYTEA);

  if (!m_stmt.execute()) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  m_conn.commit();

  // now select the data back
  m_stmt.parse("select * from bt");

  m_stmt.define(1, &v_col_1P, PG_INTEGER);
  m_stmt.define(2, &v_col_2P, PG_BYTEA);

  if (!m_stmt.execute()) {
    fprintf(stderr, "Execute Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  PgCursor *m_curr = m_stmt.getCursor();

  if (m_curr == NULL) {
    printf("Connect Error:\n\t%s\n", m_conn.getErrorMsg());
    return EXIT_FAILURE;
  }

  if (m_curr->fetch()) {
    printf("Result: %i\n", v_col_1);

    // open the file for writing in binary mode
    char *v_file_out = "/tmp/out.jpg";
    v_bin = fopen(v_file_out, "wb");
    int v_bytes_out = fwrite(v_col_2, 1, v_col_2P.getLength(), v_bin);
    printf("Wrote %i bytes of %i bytes to '%s'\n", v_bytes_out,
           v_col_2P.getLength(), v_file_out);
    fclose(v_bin);
  }

  m_curr->close();

  m_conn.disconnect();

  return EXIT_SUCCESS;
}

// =========================================================================
