// This file Copyright © 2009-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <cstdint> // int64_t
#include <map>
#include <string_view>
#include <vector>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <libtransmission/transmission.h>
#include <libtransmission/quark.h>
#include <libtransmission/tr-macros.h>

#include "RpcClient.h"
#include "RpcQueue.h"
#include "Torrent.h"
#include "Typedefs.h"

class AddData;
class Prefs;

extern "C"
{
    struct tr_variant;
}

class Session : public QObject
{
    Q_OBJECT
    TR_DISABLE_COPY_MOVE(Session)

public:
    Session(QString config_dir, Prefs& prefs);
    ~Session() override;

    void stop();
    void restart();

    QUrl const& getRemoteUrl() const
    {
        return rpc_.url();
    }

    [[nodiscard]] constexpr auto const& getStats() const noexcept
    {
        return stats_;
    }

    [[nodiscard]] constexpr auto const& getCumulativeStats() const noexcept
    {
        return cumulative_stats_;
    }

    [[nodiscard]] constexpr auto const& sessionVersion() const noexcept
    {
        return session_version_;
    }

    [[nodiscard]] constexpr auto blocklistSize() const noexcept
    {
        return blocklist_size_;
    }

    void setBlocklistSize(int64_t i);
    void updateBlocklist();
    void portTest();
    void copyMagnetLinkToClipboard(int torrent_id);

    /** returns true if the transmission session is being run inside this client */
    bool isServer() const;

    /** returns true if isServer() is true or if the remote address is the localhost */
    bool isLocal() const;

    RpcResponseFuture exec(tr_quark method, tr_variant* args);
    RpcResponseFuture exec(std::string_view method, tr_variant* args);

    using Tag = RpcQueue::Tag;
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, bool val);
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, int val);
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, double val);
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, QString const& val);
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, QList<int> const& val);
    Tag torrentSet(torrent_ids_t const& ids, tr_quark const key, QStringList const& val);

    void torrentSetLocation(torrent_ids_t const& ids, QString const& path, bool do_move);
    void torrentRenamePath(torrent_ids_t const& ids, QString const& oldpath, QString const& newname);
    void addTorrent(AddData const& addme, tr_variant* top, bool trash_original);
    void initTorrents(torrent_ids_t const& ids = {});
    void pauseTorrents(torrent_ids_t const& torrentIds = {});
    void startTorrents(torrent_ids_t const& torrentIds = {});
    void startTorrentsNow(torrent_ids_t const& torrentIds = {});
    void refreshDetailInfo(torrent_ids_t const& torrent_ids);
    void refreshActiveTorrents();
    void refreshAllTorrents();
    void addNewlyCreatedTorrent(QString const& filename, QString const& local_path);
    void verifyTorrents(torrent_ids_t const& torrent_ids);
    void reannounceTorrents(torrent_ids_t const& torrent_ids);
    void refreshExtraStats(torrent_ids_t const& ids);

    enum class TorrentProperties
    {
        MainInfo,
        MainStats,
        MainAll,
        DetailInfo,
        DetailStat,
        Rename
    };

public slots:
    void addTorrent(AddData const& addme);
    void launchWebInterface() const;
    void queueMoveBottom(torrent_ids_t const& torrentIds = {});
    void queueMoveDown(torrent_ids_t const& torrentIds = {});
    void queueMoveTop(torrent_ids_t const& torrentIds = {});
    void queueMoveUp(torrent_ids_t const& torrentIds = {});
    void refreshSessionInfo();
    void refreshSessionStats();
    void removeTorrents(torrent_ids_t const& torrent_ids, bool delete_files = false);
    void updatePref(int key);

signals:
    void sourceChanged();
    void portTested(bool is_open);
    void statsUpdated();
    void sessionUpdated();
    void blocklistUpdated(int);
    void torrentsUpdated(tr_variant* torrent_list, bool complete_list);
    void torrentsRemoved(tr_variant* torrent_list);
    void sessionCalled(Tag);
    void dataReadProgress();
    void dataSendProgress();
    void networkResponse(QNetworkReply::NetworkError code, QString const& message);
    void httpAuthenticationRequired();

private slots:
    void onDuplicatesTimer();

private:
    void start();

    void updateStats(tr_variant* args);
    void updateInfo(tr_variant* args);

    Tag torrentSetImpl(tr_variant* args);
    void sessionSet(tr_quark const key, QVariant const& variant);
    void pumpRequests();
    void sendTorrentRequest(std::string_view request, torrent_ids_t const& torrent_ids);
    void refreshTorrents(torrent_ids_t const& ids, TorrentProperties props);
    std::vector<std::string_view> const& getKeyNames(TorrentProperties props);

    static void updateStats(tr_variant* d, tr_session_stats* stats);

    void addOptionalIds(tr_variant* args, torrent_ids_t const& ids) const;

    QString const config_dir_;
    Prefs& prefs_;

    std::map<TorrentProperties, std::vector<std::string_view>> names_;

    int64_t blocklist_size_ = -1;
    tr_session* session_ = {};
    QStringList idle_json_;
    tr_session_stats stats_ = {};
    tr_session_stats cumulative_stats_ = {};
    QString session_version_;
    QString session_id_;
    bool is_definitely_local_session_ = true;
    RpcClient rpc_;
    torrent_ids_t const RecentlyActiveIDs = { -1 };

    std::map<QString, QString> duplicates_;
    QTimer duplicates_timer_;
};
