#include "file-meta-info.h"
#include "file-info-manager.h"

#include <QDebug>

using namespace Peony;

std::shared_ptr<FileMetaInfo> FileMetaInfo::fromGFileInfo(const QString &uri, GFileInfo *g_info)
{
    return std::make_shared<FileMetaInfo>(uri, g_info);
}

std::shared_ptr<FileMetaInfo> FileMetaInfo::fromUri(const QString &uri)
{
    auto mgr = FileInfoManager::getInstance();
    auto info = mgr->findFileInfoByUri(uri);
    if (info)
        return mgr->findFileInfoByUri(uri)->m_meta_info;
    return nullptr;
}

FileMetaInfo::FileMetaInfo(const QString &uri, GFileInfo *g_info)
{
    m_uri = uri;
    if (g_info) {
        char **metainfo_attributes = g_file_info_list_attributes(g_info, "metadata");
        if (metainfo_attributes) {
            for (int i = 0; metainfo_attributes[i] != nullptr; i++) {
                char *string = g_file_info_get_attribute_as_string(g_info, metainfo_attributes[i]);
                if (string) {
                    auto var = QVariant(string);
                    this->setMetaInfoVariant(metainfo_attributes[i], var);
                    //qDebug()<<"======"<<m_uri<<metainfo_attributes[i]<<var.toString();
                    g_free(string);
                }
            }
            g_strfreev(metainfo_attributes);
        }
    }
}

void FileMetaInfo::setMetaInfoInt(const QString &key, int value)
{
    setMetaInfoVariant(key, QString::number(value));
}

void FileMetaInfo::setMetaInfoString(const QString &key, const QString &value)
{
    setMetaInfoVariant(key, value);;
}

void FileMetaInfo::setMetaInfoStringList(const QString &key, const QStringList &value)
{
    QString string = value.join('\n');
    setMetaInfoVariant(key, string);
}

void FileMetaInfo::setMetaInfoVariant(const QString &key, const QVariant &value)
{
    m_mutex.lock();
    QString realKey = key;
    if (!key.startsWith("metadata::"))
        realKey = "metadata::" + key;

    m_meta_hash.remove(realKey);
    m_meta_hash.insert(realKey, value);
    GFile *file = g_file_new_for_uri(m_uri.toUtf8().constData());
    GFileInfo *info = g_file_info_new();
    std::string tmp = realKey.toStdString();
    auto data = value.toString().toUtf8().data();
    g_file_info_set_attribute(info, tmp.c_str(), G_FILE_ATTRIBUTE_TYPE_STRING, (gpointer)data);
    //FIXME: should i add callback?
    g_file_set_attributes_async(file,
                                info,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                0,
                                nullptr,
                                nullptr,
                                nullptr);
    //qDebug()<<tmp.c_str()<<value;
    GError *err = nullptr;
    g_file_set_attribute(file, tmp.c_str(), G_FILE_ATTRIBUTE_TYPE_STRING, (gpointer)data,
                         G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, &err);

    if (err) {
        qDebug()<<err->message;
        g_error_free(err);
    }
    g_object_unref(info);
    g_object_unref(file);
    m_mutex.unlock();
}

const QVariant FileMetaInfo::getMetaInfoVariant(const QString &key)
{
    QString realKey = key;
    if (!key.startsWith("metadata::"))
        realKey = "metadata::" + key;
    if (m_meta_hash.value(realKey).isValid())
        return m_meta_hash.value(realKey);
    //FIXME: should i use gio query meta here?
    return QVariant();
}

const QString FileMetaInfo::getMetaInfoString(const QString &key)
{
    return getMetaInfoVariant(key).toString();
}

const QStringList FileMetaInfo::getMetaInfoStringList(const QString &key)
{
    return getMetaInfoVariant(key).toString().split('\n');
}

int FileMetaInfo::getMetaInfoInt(const QString &key)
{
    return getMetaInfoVariant(key).toString().toInt();
}

void FileMetaInfo::removeMetaInfo(const QString &key)
{
    m_mutex.lock();
    QString realKey = key;
    if (!key.startsWith("metadata::"))
        realKey = "metadata::" + key;
    m_meta_hash.remove(realKey);
    GFile *file = g_file_new_for_uri(m_uri.toUtf8().constData());
    GFileInfo *info = g_file_info_new();
    std::string tmp = realKey.toStdString();
    g_file_info_set_attribute(info, tmp.c_str(), G_FILE_ATTRIBUTE_TYPE_STRING, nullptr);
    g_file_info_remove_attribute(info, tmp.c_str());
    //FIXME: should i add callback?
    g_file_set_attributes_async(file,
                                info,
                                G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                0,
                                nullptr,
                                nullptr,
                                nullptr);
    g_object_unref(info);
    g_object_unref(file);
    m_mutex.unlock();
}
