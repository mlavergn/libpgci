/***************************************************************************
* libpgci.h  -  A C++ parse/bind/execute compatible interface for PostgreSQL
* -------------------
* begin     : Sat Jun 15 2002
* copyright : (C) 2002 by Marc Lavergne
***************************************************************************/

/*
  NOTES:
    - PostgreSQL has an 8k limit on SQL statement size, may want to
      add in a check for that.
    - PostgreSQL, libpq to be more specific, treats all data as text
*/

// make certain we don't get included twice
#ifndef LIBPGCI_H
#define LIBPGCI_H

#undef  DEBUG
//#define DEBUG

#include <libpq-fe.h>

// =========================================================================

#define PG_VARCHAR 1
#define PG_INTEGER 2
#define PG_DATE    3
#define PG_BYTEA   4

// limits
#define MAX_INT_SIZE 10
#define MAX_BIND_CNT 512

// =========================================================================

class PgConnection
{
  public:
    PgConnection();
    bool         connect(char* p_username, char* p_password, char* p_database)
                   {return connect((unsigned char*)p_username, (unsigned char*)p_password, (unsigned char*)p_database);};
    bool         connect(unsigned char* p_username, unsigned char* p_password, unsigned char* p_database)
                   {return connect(p_username, p_password, p_database, (unsigned char*)"127.0.0.1", 5432);}
    bool         connect(unsigned char* p_username, unsigned char* p_password, unsigned char* p_database, unsigned char* p_host)
                   {return connect(p_username, p_password, p_database, p_host, 5432);}
    bool         connect(unsigned char* p_username, unsigned char* p_password, unsigned char* p_database, unsigned char* p_host, int p_port);
    bool         execute(unsigned char* p_sql);
    bool         commit();
    bool         rollback();
    void         disconnect();
    unsigned char* getErrorMsg();

    PGconn*      getPGConn() {return m_conn;}

  private:
    PGconn*      m_conn;
};

// =========================================================================

class PgPlaceholder
{
  friend class PgOperation;
  friend class PgStatement;
  friend class PgDirectLoad;
  friend class PgCursor;

  public:
    PgPlaceholder();

    PgPlaceholder(void* p_ptr, unsigned int p_len, unsigned int p_max);
    PgPlaceholder(unsigned char* p_ptr, unsigned int p_len, unsigned int p_max);
    PgPlaceholder(unsigned char* p_ptr, unsigned int p_len);
    PgPlaceholder(unsigned char* p_ptr);
    PgPlaceholder(unsigned int*  p_ptr);

    void init(void* p_ptr, unsigned int p_len, unsigned int p_max);

    ~PgPlaceholder();

    int            getIntValue()   {return *(int*)m_ptr;}
    unsigned char* getUCharValue() {return (unsigned char*)m_ptr;}
    char*          getCharValue()  {return (char*)m_ptr;}

    unsigned int   getLength()     {return m_len;}
    bool           isNull();

    void           setNull(bool p_null)          {m_null = p_null;};
    void           setLength(unsigned int p_len) {m_len  = p_len;};

    void           setMaxLength(unsigned int p_len) {m_max = p_len;};
    void           setType(int p_type) {m_type = p_type;}

  protected:
    // we don't want this public since we need info on the the type and len
    // plus BYTEAs need to be passed in escaped
    bool           setValue(char* p_ptr);

  private:
    unsigned int    m_type;
    unsigned int    m_len;
    unsigned int    m_max;
    bool            m_null;
    void*           m_ptr;
    unsigned char*  m_esc;     // escaped copy of data for bind vars only

    unsigned int    getMaxLength() {return m_max;}
    unsigned int    getType()      {return m_type;}

    unsigned char*  getEscValue()  {return m_esc;}
    unsigned char*  getItoaValue();

    // this is a prototype of the PQunescapeBytea libpq function in 7.3
    unsigned char*  PQunescapeBytea(unsigned char *from, size_t *to_length);
};

// =========================================================================

class PgCursor
{
  public:
    PgCursor(PGresult* p_res, PgPlaceholder** p_define, int p_define_cnt);
    int             getColCount();
    int             getRowCount();
    int             getRowNum();
    bool            fetch();
    void            close();

  private:
    PGresult*       m_res;
    int             m_total;
    int             m_iter;

    PgPlaceholder** m_define;
    int             m_define_cnt;
};

// =========================================================================

class PgOperation
{
  public:
    void           setConnection(PgConnection* p_conn);
    bool           bind(int p_pos, PgPlaceholder* p_placeholder, int p_type);

    void           resetBinds();
  protected:
    PgOperation();
    PgOperation(PgConnection* p_conn) {setConnection(p_conn);}

    bool           escapeBinds();

    int            m_bind_cnt;

    PgPlaceholder* m_bind[MAX_BIND_CNT];
    PGconn*        m_conn;
};

// =========================================================================

class PgStatement : public PgOperation
{
  public:
    PgStatement();
    PgStatement(PgConnection* p_conn);

    bool           parse(char* p_sql)
                     {return parse((unsigned char*)p_sql);};
    bool           parse(unsigned char* p_sql);
    bool           define(int p_pos, PgPlaceholder* p_placeholder, int p_type);
    bool           execute();
    bool           execute(char* p_sql)
                     {return execute((unsigned char*)p_sql);};
    bool           execute(unsigned char* p_sql);
    PgCursor*      getCursor();

    void           resetDefines();
  private:
    PgCursor*      m_curr;

    unsigned char* m_sql_ph; // SQL with placeholders
    int            m_sql_len;

    int            m_offset[2][MAX_BIND_CNT]; // marks positions in SQL statement for bind vars
    int            m_offset_cnt;

    int            m_define_cnt;
    PgPlaceholder* m_define[MAX_BIND_CNT];

    bool           execute(unsigned char* p_sql, bool p_noparse);
};

// =========================================================================

/**
* Implements a direct load mechanism based on PostgreSQL's COPY feature
* This is a transactionless opertion intended for batch data loading
*/

class PgDirectLoad : public PgOperation
{
  public:
    PgDirectLoad() : PgOperation() {};
    PgDirectLoad(PgConnection* p_conn) : PgOperation(p_conn) {};

    bool         initialize(char* p_table_name)
                     {return initialize((unsigned char*)p_table_name);};
    bool         initialize(unsigned char* p_table_name);
    bool         execute();
    bool         complete();
};

#endif

// =========================================================================
