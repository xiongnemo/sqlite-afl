/*
** 2018-03-01
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file implements a program used for fuzz-testing the session
** module.
**
** Usage:
**
**      sessionfuzz setup         -- Generate starter test cases
**      sessionfuzz run FILE      -- Run a test fuzz on FILE
*/

/*
** We will import the entire SQLite source file to make compiling easier
*/
#define SQLITE_DEBUG 1
#define SQLITE_THREADSAFE 0
#define SQLITE_OMIT_LOAD_EXTENSION 0
#define SQLITE_ENABLE_SESSION 1
#define SQLITE_ENABLE_PREUPDATE_HOOK 1
#include "sqlite3.c"

/* Create a test database.  This will be an in-memory database */
static const char zInitSql[] = 
  "CREATE TABLE t1(a INTEGER PRIMARY KEY,b,c,d);\n"
  "CREATE TABLE t2(e TEXT PRIMARY KEY NOT NULL,f,g);\n"
  "CREATE TABLE t3(w REAL PRIMARY KEY NOT NULL,x,y);\n"
  "CREATE TABLE t4(z PRIMARY KEY) WITHOUT ROWID;\n"
;

/* Code to populate the database */
static const char zFillSql[] = 
  "INSERT INTO t1(a,b,c,d) VALUES\n"
  "  (1,2,3,4),\n"
  "  (2,3.5,'four',x'556677'),\n"
  "  (3,null,'xyz',15),\n"
  "  (4,'bubba',0x80000000,0.0);\n"
  "INSERT INTO t1 SELECT a+4,c,d,b FROM t1;\n"
  "INSERT INTO t1 SELECT a+8,d,b,c FROM t1;\n"
  "INSERT INTO t1 SELECT a+16,d,c,b FROM t1;\n"
  "INSERT INTO t1 SELECT a+32,b,d,c FROM t1;\n"
  "INSERT INTO t2 SELECT printf('x%dy',a),b,c FROM t1;\n"
  "INSERT INTO t3 SELECT a*1.1,b,c FROM t1;\n"
  "INSERT INTO t4 SELECT a||','||quote(b) FROM t1;\n"
;

/* Help message */
static const char zHelp[] =
  "Usage:\n"
  "  sessionfuzz setup          -- Generate seed files c1.txt, c2.txt, etc.\n"
  "  sessionfuzz run FILE       -- Run against fuzzed changeset FILE\n"
;

/* Run a chunk of SQL.  If any errors happen, print an error message
** and exit.
*/
static void runSql(sqlite3 *db, const char *zSql){
  int rc;
  char *zErr = 0;
  rc = sqlite3_exec(db, zSql, 0, 0, &zErr);
  if( rc || zErr ){
    fprintf(stderr, "SQL failed: rc=%d zErr=[%s]\n", rc, zErr);
    fprintf(stderr, "SQL: [%s]\n", zSql);
    exit(1);
  }
}

/*
** Write buffer to disk
*/
static void writeFile(const char *zFilename, const void *pData, int nData){
  FILE *out;
  int n;
  out = fopen(zFilename, "wb");
  if( out==0 ){
    fprintf(stderr, "cannot open \"%s\" for writing\n", zFilename);
    exit(1);
  }
  n = (int)fwrite(pData, 1, nData, out);
  fclose(out);
  if( n!=nData ){
    fprintf(stderr, "only wrote %d of %d bytes to \"%s\"\n",n,nData,zFilename);
    exit(1);
  }
}

/*
** Generate a changeset from session pSess and write it to zFile
*/
static void makeChangeset(const char *zFile, sqlite3_session *pSess){
  void *pChg;
  int nChg;
  int rc;
  rc = sqlite3session_changeset(pSess, &nChg, &pChg);
  if( rc ){
    fprintf(stderr, "sqlite3session_changeset() returned %d\n", rc);
    exit(1);
  }
  writeFile(zFile, pChg, nChg);
  sqlite3_free(pChg);
}

/*
** Read a file from disk.  Space to hold the answer is obtained from
** sqlite3_malloc64().
*/
static void readFile(const char *zName, void **ppData, int *pnData){
  FILE *in = fopen(zName, "rb");
  long nIn;
  size_t nRead;
  char *pBuf;
  *ppData = 0;
  *pnData = 0;
  if( in==0 ){
    fprintf(stderr, "Cannot open \"%s\" for reading\n", zName);
    exit(1);
  }
  fseek(in, 0, SEEK_END);
  nIn = ftell(in);
  rewind(in);
  pBuf = sqlite3_malloc64( nIn+1 );
  if( pBuf==0 ){
    fprintf(stderr, "Failed to malloc %lld bytes\n", (sqlite3_int64)(nIn+1));
    exit(1);
  }
  nRead = fread(pBuf, 1, nIn, in);
  fclose(in);
  if( nRead!=nIn ){
    fprintf(stderr, "Read only %d of %d bytes from %s\n", (int)nRead, (int)nIn,
                    zName);
    exit(1);
  }
  pBuf[nIn] = 0;
  *pnData = nIn;
  *ppData = pBuf;
}

/*
** The conflict callback
*/
static int conflictCallback(
  void *NotUsed,
  int eConflict,
  sqlite3_changeset_iter *p
){
  (void)NotUsed;
  (void)p;
  printf("Conflict %d\n", eConflict);
  return SQLITE_CHANGESET_OMIT;
}

int main(int argc, char **argv){
  const char *zCmd;
  sqlite3 *db;
  int rc;
  sqlite3_session *pSess;
  sqlite3_stmt *pStmt;
  void *pChgset;
  int nChgset;

  if( argc<2 ){
    fprintf(stderr, "%s", zHelp);
    exit(1);
  }
  rc = sqlite3_open(":memory:",&db);
  if( rc ){
    fprintf(stderr, "Failed to open :memory: database: %s\n",
            sqlite3_errmsg(db));
    exit(1);
  }
  runSql(db, zInitSql);
  zCmd = argv[1];
  if( strcmp(zCmd, "setup")==0 ){
    if( argc!=2 ){
      fprintf(stdout, "Wrong number of arguments.\n%s", zHelp);
      exit(1);
    }
    runSql(db, zFillSql);
    rc = sqlite3session_create(db, "main", &pSess);
    if( rc ){
      fprintf(stderr, "sqlite3session_create() returns %d\n", rc);
      exit(1);
    }
    rc = sqlite3session_attach(pSess, 0);
    if( rc ){
      fprintf(stderr, "sqlite3session_attach(db,0) returns %d\n", rc);
      exit(1);
    }
    runSql(db, "INSERT INTO t4(z) VALUES('');");
    makeChangeset("c1.txt", pSess);
    runSql(db, 
      "UPDATE t1 SET b=c, c=b WHERE a IN (5,7);\n"
      "DELETE FROM t2 WHERE rowid IN (8,2);\n"
      "INSERT OR IGNORE INTO t4 SELECT b FROM t1 WHERE b IS TRUE LIMIT 2;");
    makeChangeset("c2.txt", pSess);
    runSql(db, "UPDATE t3 SET x=y, y=NULL WHERE rowid IN (1,3);");
    makeChangeset("c3.txt", pSess);
    sqlite3session_delete(pSess);
  }else
  if( strcmp(zCmd, "run")==0 ){
    int i;
    if( argc<3 ){
      fprintf(stdout, "Wrong number of arguments.\n%s", zHelp);
      exit(1);
    }
    for(i=2; i<argc; i++){
      readFile(argv[i], &pChgset, &nChgset);
      rc = sqlite3changeset_apply(db, nChgset, pChgset, 0, conflictCallback, 0);
      printf("sqlite3hangeset_apply(%s) returns %d\n", argv[i], rc);
      sqlite3_free(pChgset);
    }
  }else
  {
    fprintf(stderr, "%s", zHelp);
    exit(1);
  }
  rc = sqlite3_prepare_v2(db, "PRAGMA integrity_check;", -1, &pStmt, 0);
  if( rc ){
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    exit(1);
  }
  if( sqlite3_step(pStmt)!=SQLITE_ROW
   || strcmp((const char*)sqlite3_column_text(pStmt,0),"ok")!=0
  ){
    fprintf(stderr, "Integrity check failed!\n");
    do{
      fprintf(stderr, "%s\n", sqlite3_column_text(pStmt,0));
    }while( sqlite3_step(pStmt)==SQLITE_ROW );
  }
  sqlite3_finalize(pStmt);
  sqlite3_close(db);
  if( sqlite3_memory_used()>0 ){
    fprintf(stderr, "memory leak of %lld bytes\n",
         sqlite3_memory_used());
    exit(1);
  }
  return 0;
}
