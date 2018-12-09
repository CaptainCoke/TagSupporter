FIND_PATH(MYSQL_INCLUDES
    NAMES "mysql.h"
    PATHS
    "/usr/include/mysql"
	"${MYSQLDIR_DIR}/include/mysql"
  )
  
if (USE_EMBEDDED_SQL_DB)
    FIND_LIBRARY(MYSQL_LIB
        NAMES "mysqld"
        PATHS "/usr/lib/x86_64-linux-gnu"
    )
else(USE_EMBEDDED_SQL_DB)
    FIND_LIBRARY(MYSQL_LIB
        NAMES "mysqlclient"
        PATHS "/usr/lib/x86_64-linux-gnu"
    )
endif(USE_EMBEDDED_SQL_DB)

set(MYSQL_FOUND TRUE)