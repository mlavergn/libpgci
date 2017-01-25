/***************************************************************************
* pgci.cpp  -  A C++ parse/bind/execute compatible interface for PostgreSQL
* -------------------
* begin     : Sat Jun 15 2002
* copyright : (C) 2002 by Marc Lavergne
*
* PQunescapeBytea method (c) 1996-2001, PostgreSQL Global Development Group
***************************************************************************/

#include "libpgci.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// =========================================================================

PgConnection::PgConnection() {}

// -------------------------------------------------------------------------

bool PgConnection::connect(unsigned char *p_username, unsigned char *p_password,
                           unsigned char *p_database, unsigned char *p_host,
                           int p_port) {
  char v_conn[512];
  sprintf(&v_conn[0], "user='%s' password='%s' dbname='%s' host=%s port=%i",
          p_username, p_password, p_database, p_host, p_port);

#ifdef DEBUG
  fprintf(stderr, "Connect string:\n[%s]\n", &v_conn[0]);
#endif
  m_conn = PQconnectdb(v_conn);

  if (PQstatus(m_conn) == CONNECTION_BAD)
    return false;
  else
    return true;
}

// -------------------------------------------------------------------------

bool PgConnection::execute(unsigned char *p_sql) {
  PGresult *v_res = PQexec(m_conn, (char *)p_sql);
  if (PQresultStatus(v_res) == PGRES_TUPLES_OK ||
      PQresultStatus(v_res) == PGRES_COMMAND_OK)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

bool PgConnection::commit() {
  PGresult *v_res = PQexec(m_conn, "commit");
  if (PQresultStatus(v_res) == PGRES_COMMAND_OK)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

bool PgConnection::rollback() {
  PGresult *v_res = PQexec(m_conn, "rollback");
  if (PQresultStatus(v_res) == PGRES_COMMAND_OK)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

void PgConnection::disconnect() { PQfinish(m_conn); }

// -------------------------------------------------------------------------

unsigned char *PgConnection::getErrorMsg() {
  return (unsigned char *)PQerrorMessage(m_conn);
}

// =========================================================================

PgOperation::PgOperation() { m_bind_cnt = 0; }

// -------------------------------------------------------------------------

void PgOperation::resetBinds() {
  for (int i = 0; i < m_bind_cnt; i++) {
    free(m_bind[i]->m_esc);
    m_bind[i]->m_esc = (unsigned char *)malloc(0);
  }
  m_bind_cnt = 0;
}

// -------------------------------------------------------------------------

void PgOperation::setConnection(PgConnection *p_conn) {
  m_conn = p_conn->getPGConn();
}

// -------------------------------------------------------------------------

bool PgOperation::bind(int p_pos, PgPlaceholder *p_placeholder, int p_type) {
  if (p_pos < 1 || p_pos > 512) {
    fprintf(stderr, "NOTICE:  BIND: variable position out of range 0-512\n");
    return false;
  }

  if (p_type < 1 || p_type > 4) {
    fprintf(stderr, "NOTICE:  BIND: invalid data type %i\n", p_type);
    return false;
  }

  m_bind_cnt++;
  p_placeholder->setType(p_type);
  m_bind[p_pos - 1] = p_placeholder;

  return true;
}

// -------------------------------------------------------------------------

bool PgOperation::escapeBinds() {
  // escape any text or binary data
  for (int i = 0; i < m_bind_cnt; i++) {
    // handle null conditions
    if (m_bind[i]->isNull())
      continue;

    switch (m_bind[i]->getType()) {
    case PG_INTEGER:
      // nothing to escape
      break;
    case PG_DATE:
    case PG_VARCHAR:
      // PQescapeString makes no provision for allocating space for the escape
      // target pointer
      // the code states to allocate a buffer of len*2+1
      m_bind[i]->m_esc =
          (unsigned char *)realloc(m_bind[i]->m_esc, m_bind[i]->m_len * 2 + 1);
      m_bind[i]->m_esc[0] = 0; // NULL terminate
      m_bind[i]->m_len = PQescapeString(
          (char *)m_bind[i]->m_esc, (char *)m_bind[i]->m_ptr, m_bind[i]->m_len);
      break;
    case PG_BYTEA:
      free(m_bind[i]->m_esc);
      m_bind[i]->m_esc =
          PQescapeBytea((unsigned char *)m_bind[i]->m_ptr, m_bind[i]->m_len,
                        (size_t *)&m_bind[i]->m_len);
      break;
    default:
      return false;
    }
  }

  return true;
}

// =========================================================================

PgStatement::PgStatement() : PgOperation() {
  m_sql_ph = (unsigned char *)malloc(0);
  m_define_cnt = 0;
  m_curr = NULL;
}

// -------------------------------------------------------------------------

PgStatement::PgStatement(PgConnection *p_conn) : PgOperation(p_conn) {
  m_sql_ph = (unsigned char *)malloc(0);
  m_define_cnt = 0;
  m_curr = NULL;
}

// -------------------------------------------------------------------------

void PgStatement::resetDefines() {
  for (int i = 0; i < m_define_cnt; i++)
    if (m_define[i]->m_esc != NULL)
      free(m_bind[i]->m_esc);
  m_define_cnt = 0;
}

// -------------------------------------------------------------------------

bool PgStatement::parse(unsigned char *p_sql) {
  m_sql_len = strlen((char *)p_sql);
  int i = 0;

  m_sql_ph = (unsigned char *)realloc(m_sql_ph, m_sql_len + 1);
  memset(m_sql_ph, 0, m_sql_len + 1);

  // copy the l-trim value of the SQL statement
  while (i < m_sql_len && p_sql[i] == ' ') {
    i++;
  }
  strcpy((char *)m_sql_ph, (char *)&p_sql[i]);

  // declare the token pointer
  unsigned char *v_tok = m_sql_ph;

  m_offset_cnt = 0;
  m_offset[0][m_offset_cnt] = 0;

  while (true) {
    // detected the START of the placeholder
    v_tok = (unsigned char *)strchr((char *)v_tok, ':');

    // set the length to the remaining string length
    if (v_tok == NULL) {
      m_offset[1][m_offset_cnt] = m_sql_len - m_offset[0][m_offset_cnt];
      break;
    } else {
      m_offset[1][m_offset_cnt] = v_tok - m_sql_ph - m_offset[0][m_offset_cnt];
    }

    // detected the END of the placeholder
    v_tok = (unsigned char *)strpbrk((char *)v_tok, " ,)");

    // if this is true, we are are the end of the string so isolate the string
    // NULL terminator
    if (v_tok == NULL) {
      m_offset[0][++m_offset_cnt] = m_sql_len;
      m_offset[1][m_offset_cnt] = 1;
      break;
    } else {
      m_offset[0][++m_offset_cnt] = v_tok - m_sql_ph;
    }
  }

#ifdef DEBUG
  fprintf(stderr, "Bind variables identified: [%i]\nOffsets: ", m_offset_cnt);
  for (int _i = 0; _i <= m_offset_cnt; _i++)
    fprintf(stderr, "[%i][%i] ", m_offset[0][_i], m_offset[1][_i]);
  fprintf(stderr, "\n");
#endif

  m_define_cnt = 0;
  m_bind_cnt = 0;

  return true;
}

// -------------------------------------------------------------------------

bool PgStatement::define(int p_pos, PgPlaceholder *p_placeholder, int p_type) {
  if (p_pos < 1 || p_pos > 512) {
    fprintf(stderr, "NOTICE:  DEFINE: variable position out of range 0-512\n");
    return false;
  }

  if (p_type < 1 || p_type > 4) {
    fprintf(stderr, "NOTICE:  DEFINE: invalid data type %i\n", p_type);
    return false;
  }

  m_define_cnt++;
  p_placeholder->setType(p_type);
  m_define[p_pos - 1] = p_placeholder;

  return true;
}

// -------------------------------------------------------------------------

bool PgStatement::execute() { return execute(m_sql_ph, false); }

// -------------------------------------------------------------------------

bool PgStatement::execute(unsigned char *p_sql) { return execute(p_sql, true); }

// -------------------------------------------------------------------------

bool PgStatement::execute(unsigned char *p_sql, bool p_noparse) {
  if (!escapeBinds())
    return false;

  unsigned char *v_sql = p_sql;

  // create a transaction
  PGresult *v_res;

  // NOTE: it is not feasible to convert to lowercase to facilitate string
  // parsing
  // since hard coded variables may exost in the SQL statement

  // We only initialize a transaction if we have am INSERT or UPDATE situation
  if ((v_sql[0] == 'U' || v_sql[0] == 'u') ||
      (v_sql[0] == 'I' || v_sql[0] == 'i')) {
#ifdef DEBUG
    fprintf(stderr, "Initializing transaction\n");
#endif
    v_res = PQexec(m_conn, "begin");
    if (PQresultStatus(v_res) != PGRES_COMMAND_OK) {
      fprintf(stderr,
              "ERROR: Unable to initiate transaction (PgStatement.execute)\n");
      return false;
    }
  }

  if (!p_noparse) {
    // determine how much memory we need
    unsigned int v_bind_len = m_sql_len;
    for (int i = 0; i < m_bind_cnt; i++)
      v_bind_len += m_bind[i]->getLength();

    // null fill our SQL buffer
    v_sql = (unsigned char *)alloca(v_bind_len + 1);
    memset(v_sql, 0, v_bind_len + 1);

    if (m_bind_cnt > 0) {
      // check to make certain there are no other bind vars left
      if (m_bind_cnt != m_offset_cnt) {
        fprintf(stderr, "ERROR:  Bind variable count %i differs from bind "
                        "placeholder count %i (PgStatement.execute)\n",
                m_bind_cnt, m_offset_cnt);
        return false;
      }

      for (int i = 0; i < m_bind_cnt; i++) {
        // handle null conditions
        if (m_bind[i]->isNull()) {
          strncat((char *)v_sql, (char *)&m_sql_ph[m_offset[0][i]],
                  m_offset[1][i]);
          strcat((char *)v_sql, "NULL"); // inefficient
          continue;
        }

        switch (m_bind[i]->getType()) {
        case PG_INTEGER:
          strncat((char *)v_sql, (char *)&m_sql_ph[m_offset[0][i]],
                  m_offset[1][i]);
          strcat((char *)v_sql, (char *)m_bind[i]->getItoaValue());
          break;
        case PG_VARCHAR:
        case PG_DATE:
        case PG_BYTEA:
          strncat((char *)v_sql, (char *)&m_sql_ph[m_offset[0][i]],
                  m_offset[1][i]);
          strcat((char *)v_sql, "'"); // inefficient
          strcat((char *)v_sql, (char *)m_bind[i]->getEscValue());
          strcat((char *)v_sql, "'"); // inefficient
          break;
        default:
          return false;
        }
      }

      strncat((char *)v_sql, (char *)&m_sql_ph[m_offset[0][m_offset_cnt]],
              m_offset[1][m_offset_cnt]);
    } else
      strcpy((char *)v_sql, (char *)m_sql_ph);
  }

#ifdef DEBUG
  // Commented out since testing Byteas creates terminal noise due
  // to escaped chars.
  fprintf(stderr, "Execute string:\n[%s]\n", v_sql);
#endif

  v_res = PQexec(m_conn, (char *)v_sql);

  if (PQresultStatus(v_res) == PGRES_TUPLES_OK) {
    if (m_curr != NULL)
      delete m_curr;

    m_curr = new PgCursor(v_res, m_define, m_define_cnt);
    return true;
  } else if (PQresultStatus(v_res) == PGRES_COMMAND_OK)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

PgCursor *PgStatement::getCursor() { return m_curr; }

// =========================================================================

PgCursor::PgCursor(PGresult *p_res, PgPlaceholder **p_define,
                   int p_define_cnt) {
  m_res = p_res;

  // get the number of rows
  m_total = PQntuples(m_res);

  // set the iterrator to zero
  m_iter = 0;

  // set the define buffers
  m_define = p_define;
  m_define_cnt = p_define_cnt;
}

// -------------------------------------------------------------------------

int PgCursor::getColCount() { return PQnfields(m_res); }

// -------------------------------------------------------------------------

int PgCursor::getRowCount() { return m_total; }

// -------------------------------------------------------------------------

int PgCursor::getRowNum() { return m_iter; }

// -------------------------------------------------------------------------

bool PgCursor::fetch() {
  if (m_iter == m_total)
    return false;

  // set the pointer to statement **
  for (int i = 0; i < m_define_cnt; i++) {
    m_define[i]->setLength((unsigned int)PQgetlength(m_res, m_iter, i));
    m_define[i]->setNull(PQgetisnull(m_res, m_iter, i));
    m_define[i]->setValue(PQgetvalue(m_res, m_iter, i));
  }

  m_iter++;

  return true;
}

// -------------------------------------------------------------------------

void PgCursor::close() { return PQclear(m_res); }

// =========================================================================

PgPlaceholder::PgPlaceholder() { init(NULL, 0, 0); }

// -------------------------------------------------------------------------

PgPlaceholder::PgPlaceholder(void *p_ptr, unsigned int p_len,
                             unsigned int p_max) {
  init(p_ptr, p_len, p_max);
}

// -------------------------------------------------------------------------

PgPlaceholder::PgPlaceholder(unsigned char *p_ptr, unsigned int p_len,
                             unsigned int p_max) {
  init((void *)p_ptr, p_len, p_max);
}

// -------------------------------------------------------------------------

PgPlaceholder::PgPlaceholder(unsigned char *p_ptr, unsigned int p_len) {
  init((void *)p_ptr, p_len, p_len);
}

// -------------------------------------------------------------------------

PgPlaceholder::PgPlaceholder(unsigned char *p_ptr) {
  int v_len = strlen((char *)p_ptr);
  init((void *)p_ptr, v_len, v_len);
}

// -------------------------------------------------------------------------

PgPlaceholder::PgPlaceholder(unsigned int *p_ptr) {
  init((void *)p_ptr, sizeof(int), sizeof(int));
}

// -------------------------------------------------------------------------

void PgPlaceholder::init(void *p_ptr, unsigned int p_len, unsigned int p_max) {
  m_ptr = p_ptr;
  m_len = p_len;
  m_max = p_max;

  // we malloc 0 length here to allow us to use realloc
  m_esc = (unsigned char *)malloc(0);
}

// -------------------------------------------------------------------------

PgPlaceholder::~PgPlaceholder() { free(m_esc); }

// -------------------------------------------------------------------------

bool PgPlaceholder::setValue(char *p_ptr) {
  unsigned int v_len;
  unsigned char *v_ptr;
  switch (m_type) {
  // -----------------------------------------------------------------------
  case PG_VARCHAR:
  case PG_DATE:
    memset(m_ptr, 0, getMaxLength());
    v_len = strlen(p_ptr);
    if (v_len >= getMaxLength()) {
      strncpy((char *)m_ptr, p_ptr, getMaxLength() - 1);
      ((char *)m_ptr)[getMaxLength() - 1] = 0;
    } else
      strcpy((char *)m_ptr, p_ptr);
    break;
  // -----------------------------------------------------------------------
  case PG_INTEGER:
    *(int *)m_ptr = atoi(p_ptr);
    break;
  // -----------------------------------------------------------------------
  case PG_BYTEA:
    memset(m_ptr, 0, getMaxLength());

    v_ptr = PQunescapeBytea((unsigned char *)p_ptr, (size_t *)&m_len);

    if (m_len >= getMaxLength())
      memcpy(m_ptr, v_ptr, getMaxLength());
    else
      memcpy(m_ptr, v_ptr, m_len);

    // free the mem returned from PQunescapeBytea
    free(v_ptr);
    break;
  // -----------------------------------------------------------------------
  default:
    return false;
    // -----------------------------------------------------------------------
  }

  return true;
}

// -------------------------------------------------------------------------

unsigned char *PgPlaceholder::getItoaValue() {
  if (realloc((void *)m_esc, MAX_INT_SIZE + 1)) {
    m_esc[0] = 0; // NULL terminate
    sprintf((char *)m_esc, "%i", getIntValue());
  }

  return m_esc;
}

// -------------------------------------------------------------------------

bool PgPlaceholder::isNull() { return m_null; }

// -------------------------------------------------------------------------

// this is a prototype of the PQunescapeBytea libpq function in 7.3
// the code was taken from fe-exec.c rev 1.119 and casting was added
// to allow it to compile cleanly
unsigned char *PgPlaceholder::PQunescapeBytea(unsigned char *strtext,
                                              size_t *retbuflen) {
  size_t buflen;
  unsigned char *buffer, *sp, *bp;
  unsigned int state = 0;

  if (strtext == NULL)
    return NULL;
  buflen = strlen(
      (const char *)strtext); /* will shrink, also we discover if strtext */
  buffer = (unsigned char *)malloc(buflen); /* isn't NULL terminated */
  if (buffer == NULL)
    return NULL;
  for (bp = buffer, sp = strtext; *sp != '\0'; bp++, sp++) {
    switch (state) {
    case 0:
      if (*sp == '\\')
        state = 1;
      *bp = *sp;
      break;
    case 1:
      if (*sp == '\'') /* state=5 */
      {                /* replace \' with 39 */
        bp--;
        *bp = '\'';
        buflen--;
        state = 0;
      } else if (*sp == '\\') /* state=6 */
      {                       /* replace \\ with 92 */
        bp--;
        *bp = '\\';
        buflen--;
        state = 0;
      } else {
        if (isdigit(*sp))
          state = 2;
        else
          state = 0;
        *bp = *sp;
      }
      break;
    case 2:
      if (isdigit(*sp))
        state = 3;
      else
        state = 0;
      *bp = *sp;
      break;
    case 3:
      if (isdigit(*sp)) /* state=4 */
      {
        int v;
        bp -= 3;
        sscanf((const char *)sp - 2, "%03o", &v);
        *bp = v;
        buflen -= 3;
        state = 0;
      } else {
        *bp = *sp;
        state = 0;
      }
      break;
    }
  }

  buffer = (unsigned char *)realloc(buffer, buflen);

  if (buffer == NULL)
    return NULL;

  *retbuflen = buflen;
  return buffer;
}

// =========================================================================

bool PgDirectLoad::initialize(unsigned char *p_table_name) {
  PGresult *v_res;

  if (p_table_name == NULL)
    return false;

  char *v_sql = (char *)alloca(17 + strlen((char *)p_table_name));
  v_sql[0] = 0;
  sprintf(v_sql, "copy %s from stdin", p_table_name);

  m_bind_cnt = 0;

  v_res = PQexec(m_conn, v_sql);

  if (PQresultStatus(v_res) == PGRES_COPY_IN)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

bool PgDirectLoad::execute() {
  // PQescapeString makes no provision for escaping newlines or tabs, this give
  // the COPY mechanism
  // a real hangover (eg: CopyReadNewline: extra fields ignored) so we need to
  // escape those
  // ourselves, the proper thing appears to be \n -> \ + n. Do NOT use
  // PQescapeString, it really
  // doesn't work with COPY at all

  for (int i = 0; i < m_bind_cnt; i++) {
    // handle null conditions
    if (m_bind[i]->isNull())
      continue;

    int v_off = 0;
    m_bind[i]->m_esc =
        (unsigned char *)realloc(m_bind[i]->m_esc, m_bind[i]->m_len * 2 + 1);

    // adpated from /src/backend/commands/copy.c - CopyReadAttribute
    for (unsigned int j = 0; j < m_bind[i]->m_len; j++)
      switch (((char *)m_bind[i]->m_ptr)[j]) {
      case '\b':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 'b';
        break;
      case '\f':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 'f';
        break;
      case '\n':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 'n';
        break;
      case '\r':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 'r';
        break;
      case '\t':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 't';
        break;
      case '\v':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = 'v';
        break;
      case '\\':
        m_bind[i]->m_esc[v_off++] = '\\';
        m_bind[i]->m_esc[v_off++] = '\\';
        break;
      default:
        m_bind[i]->m_esc[v_off++] = ((char *)m_bind[i]->m_ptr)[j];
      }
    m_bind[i]->m_esc[v_off] = (char)NULL;
    m_bind[i]->m_len = v_off;
  }

#ifdef DEBUG
  fprintf(stderr, "Execute bind count: [%03i]\n", m_bind_cnt);
#endif

  int v_len = 1; // space for NULL terminator
  for (int i = 0; i < m_bind_cnt; i++)
    v_len += m_bind[i]->getLength() + 1;

  char *v_sql = (char *)alloca(v_len);
  v_sql[0] = 0;

  for (int i = 0; i < m_bind_cnt; i++) {
    // only go in if value is not null
    if (m_bind[i]->isNull())
      strcat(v_sql, "\\N"); // NULL marker
    else
      switch (m_bind[i]->getType()) {
      case PG_INTEGER:
        strcat(v_sql, (char *)m_bind[i]->getItoaValue());
        break;
      case PG_VARCHAR:
      case PG_DATE:
      case PG_BYTEA:
        strcat(v_sql, (char *)m_bind[i]->getEscValue());
        break;
      default:
        return false;
      }

    if (i < m_bind_cnt - 1)
      strcat(v_sql, "\t"); // TAB between values
    else
      strcat(v_sql, "\n"); // NEWLINE at the end
  }

#ifdef DEBUG
  fprintf(stderr, "Execute direct: [%s]\n", v_sql);
#endif

  int v_rc = PQputline(m_conn, v_sql);

  if (v_rc == 1)
    return true;
  else
    return false;
}

// -------------------------------------------------------------------------

bool PgDirectLoad::complete() {
  PQputline(m_conn, "\\.\n");
  int v_rc = PQendcopy(m_conn);

  if (v_rc == 1)
    return true;
  else
    return false;
}

// =========================================================================
