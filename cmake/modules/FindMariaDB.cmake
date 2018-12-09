find_path(MARIADB_INCLUDES
    NAMES
    mysql.h
    PATHS
	"${MARIADB_DIR}/include/mysql"
	${INCLUDE_INSTALL_DIR}
  )
  
SET(args PATHS ${LIB_INSTALL_DIR} "${MARIADB_DIR}/lib")             
IF(NOT WIN32)
      # on non-win32 we don't need to take care about WIN32_DEBUG_POSTFIX

      FIND_LIBRARY(MARIADB_LIBRARIES libmariadb ${args})
	  
ELSE(NOT WIN32)
   # search the release lib
      FIND_LIBRARY(MARIADB_LIBRARIES
                   NAMES "libmariadb"
                   ${args} 
      )
ENDIF(NOT WIN32)

set(MARIADB_FOUND TRUE)