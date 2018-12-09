#include "TemporaryRecursiveCopy.h"
#include <QDir>

TemporaryRecursiveCopy::TemporaryRecursiveCopy( const QString& strSrcPath, const QString& strTemporaryBasePath )
{
    // create temporary base path (if it doesn't exist yet)
    QDir().mkpath(strTemporaryBasePath);
    QDir cl_dst_dir(strTemporaryBasePath);
    if ( !cl_dst_dir.exists() )
        throw std::runtime_error( "failed to create destination directory" );
            
    QStringList lst_files_to_copy;
    QFileInfo cl_src_info( strSrcPath );
    if ( cl_src_info.isDir() )
    {
        QDir cl_src_dir(strSrcPath);
        for ( const QString& str_filename : cl_src_dir.entryList(QDir::Files) )
            lst_files_to_copy << cl_src_dir.filePath( str_filename );
        
        for( const QString & str_dir : cl_src_dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot) )
            m_lstSubDirs.emplace_back( cl_src_dir.filePath( str_dir ), cl_dst_dir.filePath( str_dir ) );
    }
    else if ( cl_src_info.isFile() )
        lst_files_to_copy << strSrcPath;
    
    // copy all files in current level of dir-tree
    for( const QString & str_full_src_path : lst_files_to_copy )
    {
        QString str_full_dst_path = cl_dst_dir.filePath( QFileInfo(str_full_src_path).fileName() );
        if( QFile::copy( str_full_src_path, str_full_dst_path ) )
            m_lstCopiedFiles << str_full_dst_path;
    }
}

TemporaryRecursiveCopy::~TemporaryRecursiveCopy()
{
    // erase all temporary files that were created by copying
    for ( const QString & str_full_path : m_lstCopiedFiles )
        QFile::remove( str_full_path );
}
