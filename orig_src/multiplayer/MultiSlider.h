#ifndef MULTIPLAYER_MULTISLIDER_H
#define MULTIPLAYER_MULTISLIDER_H

#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace multislider
{
    template <typename T>
    using shared_ptr = std::shared_ptr<T>;

    class RuntimeError : public std::runtime_error
    {
    public:
        explicit RuntimeError(const std::string& what) : std::runtime_error(what) {}
    };

    struct Blob
    {
        std::string data;
    };

    typedef std::map<std::string, Blob> SessionData;
    typedef Blob PlayerData;

    class Session;
    class SessionCallback
    {
    public:
        virtual ~SessionCallback() {}
        virtual void onStart(Session* session) { (void)session; }
        virtual void onUpdate(Session* session, const SessionData& data, const PlayerData& sharedData)
        {
            (void)session;
            (void)data;
            (void)sharedData;
        }
        virtual void onQuit(Session* session, const std::string& playerName, bool byTimeout) throw ()
        {
            (void)session;
            (void)playerName;
            (void)byTimeout;
        }
    };

    class RoomInfo
    {
    public:
        const std::string& getName() const { return mName; }
        const std::string& getDescription() const { return mDescription; }
        const std::string& getHostName() const { return mHostName; }
        size_t getPlayersNumber() const { return mPlayers; }
        size_t getReservedPlayersNumber() const { return mReservedPlayers; }
        const std::vector<std::string>& getPlayers() const { return mPlayerNames; }

    private:
        std::string mName;
        std::string mDescription;
        std::string mHostName;
        size_t mPlayers = 0;
        size_t mReservedPlayers = 0;
        std::vector<std::string> mPlayerNames;

        friend class Lobby;
    };

    class Lobby
    {
    public:
        enum Status
        {
            SUCCESS = 0,
            ERROR = 1
        };

        enum
        {
            FLAG_IS_EJECTED = 1 << 0,
            FLAG_JOINED = 1 << 1,
            FLAG_LEFT = 1 << 2,
            FLAG_NEW_HOST = 1 << 3,
            FLAG_RECONFIGURED_BY_HOST = 1 << 4,
            FLAG_RECONFIGURE_FAIL = 1 << 5,
            FLAG_ROOM_CLOSED_BY_HOST = 1 << 6
        };

        class Callback
        {
        public:
            virtual ~Callback() {}
            virtual void onJoined(Lobby* lobby, const RoomInfo& room) { (void)lobby; (void)room; }
            virtual void onLeft(Lobby* lobby, const RoomInfo& room, uint8_t flags) { (void)lobby; (void)room; (void)flags; }
            virtual void onMessage(Lobby* lobby, const RoomInfo& room, const std::string& sender, const std::string& message)
            {
                (void)lobby;
                (void)room;
                (void)sender;
                (void)message;
            }
            virtual void onRoomUpdate(Lobby* lobby, const RoomInfo& room, const std::string& sender, uint8_t flags)
            {
                (void)lobby;
                (void)room;
                (void)sender;
                (void)flags;
            }
            virtual void onSessionStart(Lobby* lobby, const RoomInfo& room, std::shared_ptr<Session> session, const std::string& sessionData)
            {
                (void)lobby;
                (void)room;
                (void)session;
                (void)sessionData;
            }
        };

        Lobby(const std::string& ip, uint16_t port) : mIp(ip), mPort(port), mIsHost(false), mLastPing(0) {}

        static std::vector<RoomInfo> getRooms(const std::string& ip, std::uint16_t port)
        {
            (void)ip;
            (void)port;
            return std::vector<RoomInfo>();
        }

        Status createRoom(const std::string& userName, const std::string& roomName, const std::string& version, uint32_t playersLimit, uint32_t aiAmount, Callback* cb)
        {
            (void)version;
            (void)playersLimit;
            (void)aiAmount;
            mPlayerName = userName;
            mRoom = RoomInfo();
            mRoom.mName = roomName;
            mRoom.mHostName = userName;
            mRoom.mPlayerNames.push_back(userName);
            mIsHost = true;
            if (cb) cb->onJoined(this, mRoom);
            return SUCCESS;
        }

        Status joinRoom(const std::string& userName, const RoomInfo& room, Callback* cb)
        {
            mPlayerName = userName;
            mRoom = room;
            mIsHost = false;
            if (cb) cb->onJoined(this, mRoom);
            return SUCCESS;
        }

        void receive() {}
        void say(const std::string& message, bool sendToSelf) { (void)message; (void)sendToSelf; }
        void pollPing() {}
        size_t getLastPing() const { return mLastPing; }
        bool isHost() const { return mIsHost; }
        const RoomInfo& getRoomInfo() const { return mRoom; }
        const std::string& getPlayerName() const { return mPlayerName; }
        void startSession(const std::string& sessionData) { (void)sessionData; }
        void reconfigure(uint32_t playersLimit, uint32_t aiAmount) { (void)playersLimit; (void)aiAmount; }

    private:
        std::string mIp;
        uint16_t mPort;
        bool mIsHost;
        size_t mLastPing;
        std::string mPlayerName;
        RoomInfo mRoom;
    };

    class Session
    {
    public:
        void startup(SessionCallback* cb, uint32_t timeoutMs)
        {
            (void)timeoutMs;
            mCallback = cb;
            if (mCallback) mCallback->onStart(this);
        }
        void receive() {}
        void broadcast(const std::string& data, const std::string& sharedData, bool reliable)
        {
            (void)data;
            (void)sharedData;
            (void)reliable;
        }
        uint64_t getLastPing() const { return 0; }

    private:
        SessionCallback* mCallback = 0;
    };

    typedef std::shared_ptr<Session> SessionPtr;
}

#endif
