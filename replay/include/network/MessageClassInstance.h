#pragma once


class MessageClassState {
private:
  int numClassIdBits = -1;
  enum class MessageSysSyncState : uint8_t
  {
    SERVER,         // sync in server mode
    NOT_SYNCED,     // client - not using message sync, it is either unused, or not yet connected to server
    WAITING_SYNC,   // client - connected to server, sent hashes, and waiting for sync data
    SYNCED_MATCHED, // client - synced, no data received, because messages on client and server match
    SYNCED_FULL,    // client - synced, server message data is received
    SYNC_FAILED,    // client - sync has failed
  };
  MessageSysSyncState msgSysSyncState = MessageSysSyncState::NOT_SYNCED;
  uint64_t allMessagesHash = 0u;

};