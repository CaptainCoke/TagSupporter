#ifndef TEMPORARYRECURSIVECOPY_H
#define TEMPORARYRECURSIVECOPY_H

#include <list>
#include <QStringList>

class TemporaryRecursiveCopy
{
public:
    TemporaryRecursiveCopy( const QString& strSrcPath, const QString& strTemporaryBasePath );
    ~TemporaryRecursiveCopy();
  
protected:
    QStringList m_lstCopiedFiles;
    std::list<TemporaryRecursiveCopy> m_lstSubDirs;
};

#endif
