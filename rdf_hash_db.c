#define db_DELETE(db, key, flags)       ((db->dbp)->del)(db->dbp, TXN &key, flag
s)
#define db_STORE(db, key, value, flags) ((db->dbp)->put)(db->dbp, TXN &key, &val
ue, flags)
#define db_FETCH(db, key, flags)        ((db->dbp)->get)(db->dbp, TXN &key, &val
ue, flags)

#define db_sync(db, flags)              ((db->dbp)->sync)(db->dbp, flags)
#define db_get(db, key, value, flags)   ((db->dbp)->get)(db->dbp, TXN &key, &val
ue, flags)

#ifdef DB_VERSION_MAJOR
#define db_DESTROY(db)                  ( db->cursor->c_close(db->cursor),\
                                          (db->dbp->close)(db->dbp, 0) )
#define db_close(db)                    ((db->dbp)->close)(db->dbp, 0)
#define db_del(db, key, flags)          (flagSet(flags, R_CURSOR)               
                        \
                                                ? ((db->cursor)->c_del)(db->curs
or, 0)          \
                                                : ((db->dbp)->del)(db->dbp, NULL
, &key, flags) )


put
