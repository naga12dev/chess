const { WebSocketServer } = require('ws');
const { Chess } = require('chess.js');

const PORT = process.env.PORT || 3000;

// In-memory game rooms
const rooms = new Map();

class Room {
  constructor(code, hostColor) {
    this.code = code;
    this.hostColor = hostColor;
    this.host = null;         // { ws, color, connected }
    this.guest = null;        // { ws, color, connected }
    this.chess = new Chess();
    this.moveHistory = [];    // [{ from, to, promotion, color }]
    this.status = 'waiting';  // 'waiting' | 'active' | 'finished'
    this.lastActivity = Date.now();
    this.disconnectTimers = new Map(); // role → setTimeout handle
  }

  getPlayerByWs(ws) {
    if (this.host && this.host.ws === ws) return 'host';
    if (this.guest && this.guest.ws === ws) return 'guest';
    return null;
  }

  getPlayer(role) {
    return role === 'host' ? this.host : this.guest;
  }

  getOpponentRole(role) {
    return role === 'host' ? 'guest' : 'host';
  }

  getColorForRole(role) {
    if (role === 'host') return this.hostColor;
    return this.hostColor === 'white' ? 'black' : 'white';
  }

  broadcast(msg) {
    const data = JSON.stringify(msg);
    if (this.host && this.host.ws && this.host.ws.readyState === 1) {
      this.host.ws.send(data);
    }
    if (this.guest && this.guest.ws && this.guest.ws.readyState === 1) {
      this.guest.ws.send(data);
    }
  }

  sendTo(role, msg) {
    const player = this.getPlayer(role);
    if (player && player.ws && player.ws.readyState === 1) {
      player.ws.send(JSON.stringify(msg));
    }
  }
}

function generateRoomCode() {
  const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'; // No I, O, 0, 1 to avoid confusion
  let code;
  do {
    code = '';
    for (let i = 0; i < 6; i++) {
      code += chars[Math.floor(Math.random() * chars.length)];
    }
  } while (rooms.has(code));
  return code;
}

function send(ws, msg) {
  if (ws && ws.readyState === 1) {
    ws.send(JSON.stringify(msg));
  }
}

// Map ws → { roomCode, role }
const connections = new Map();

function handleMessage(ws, data) {
  let msg;
  try {
    msg = JSON.parse(data);
  } catch {
    send(ws, { type: 'error', message: 'Invalid JSON' });
    return;
  }

  switch (msg.type) {
    case 'create_room':
      handleCreateRoom(ws, msg);
      break;
    case 'join_room':
      handleJoinRoom(ws, msg);
      break;
    case 'move':
      handleMove(ws, msg);
      break;
    case 'resign':
      handleResign(ws);
      break;
    case 'ping':
      send(ws, { type: 'pong' });
      break;
    default:
      send(ws, { type: 'error', message: 'Unknown message type' });
  }
}

function handleCreateRoom(ws, msg) {
  // Clean up any existing connection
  cleanupConnection(ws);

  const color = msg.color === 'black' ? 'black' : 'white';
  const code = generateRoomCode();
  const room = new Room(code, color);
  room.host = { ws, color, connected: true };
  rooms.set(code, room);
  connections.set(ws, { roomCode: code, role: 'host' });

  send(ws, { type: 'room_created', code, color });
  console.log(`Room ${code} created by host (${color})`);
}

function handleJoinRoom(ws, msg) {
  const code = (msg.code || '').toUpperCase().trim();
  const room = rooms.get(code);

  if (!room) {
    send(ws, { type: 'error', message: 'Room not found' });
    return;
  }

  // Check if this is a reconnection
  const existingConn = connections.get(ws);

  if (room.status === 'active' || room.status === 'waiting') {
    // Check for reconnection: if a player disconnected, allow rejoin
    if (room.host && !room.host.connected && room.status === 'active') {
      // Reconnect as host
      cleanupConnection(ws);
      room.host.ws = ws;
      room.host.connected = true;
      connections.set(ws, { roomCode: code, role: 'host' });

      // Cancel disconnect timer
      if (room.disconnectTimers.has('host')) {
        clearTimeout(room.disconnectTimers.get('host'));
        room.disconnectTimers.delete('host');
      }

      send(ws, {
        type: 'reconnected',
        color: room.hostColor,
        fen: room.chess.fen(),
        moveHistory: room.moveHistory,
        currentTurn: room.chess.turn() === 'w' ? 'white' : 'black'
      });
      room.sendTo('guest', { type: 'opponent_reconnected' });
      console.log(`Host reconnected to room ${code}`);
      return;
    }

    if (room.guest && !room.guest.connected && room.status === 'active') {
      // Reconnect as guest
      cleanupConnection(ws);
      const guestColor = room.hostColor === 'white' ? 'black' : 'white';
      room.guest.ws = ws;
      room.guest.connected = true;
      connections.set(ws, { roomCode: code, role: 'guest' });

      // Cancel disconnect timer
      if (room.disconnectTimers.has('guest')) {
        clearTimeout(room.disconnectTimers.get('guest'));
        room.disconnectTimers.delete('guest');
      }

      send(ws, {
        type: 'reconnected',
        color: guestColor,
        fen: room.chess.fen(),
        moveHistory: room.moveHistory,
        currentTurn: room.chess.turn() === 'w' ? 'white' : 'black'
      });
      room.sendTo('host', { type: 'opponent_reconnected' });
      console.log(`Guest reconnected to room ${code}`);
      return;
    }

    // Normal join (new guest)
    if (room.status !== 'waiting') {
      send(ws, { type: 'error', message: 'Room is full' });
      return;
    }

    cleanupConnection(ws);
    const guestColor = room.hostColor === 'white' ? 'black' : 'white';
    room.guest = { ws, color: guestColor, connected: true };
    room.status = 'active';
    connections.set(ws, { roomCode: code, role: 'guest' });

    send(ws, {
      type: 'room_joined',
      color: guestColor,
      fen: room.chess.fen(),
      moveHistory: room.moveHistory,
      currentTurn: room.chess.turn() === 'w' ? 'white' : 'black'
    });
    room.sendTo('host', { type: 'opponent_joined' });
    console.log(`Guest joined room ${code} (${guestColor})`);
  } else {
    send(ws, { type: 'error', message: 'Game has ended' });
  }
}

function handleMove(ws, msg) {
  const conn = connections.get(ws);
  if (!conn) {
    send(ws, { type: 'error', message: 'Not in a room' });
    return;
  }

  const room = rooms.get(conn.roomCode);
  if (!room || room.status !== 'active') {
    send(ws, { type: 'error', message: 'Game not active' });
    return;
  }

  // Verify it's this player's turn
  const playerColor = room.getColorForRole(conn.role);
  const currentTurn = room.chess.turn() === 'w' ? 'white' : 'black';
  if (playerColor !== currentTurn) {
    send(ws, { type: 'invalid_move', reason: 'Not your turn' });
    return;
  }

  // Attempt the move using chess.js
  const moveObj = {
    from: msg.from,
    to: msg.to,
  };
  if (msg.promotion) {
    // Map full piece names to chess.js single chars
    const promoMap = { queen: 'q', rook: 'r', bishop: 'b', knight: 'n', q: 'q', r: 'r', b: 'b', n: 'n' };
    moveObj.promotion = promoMap[msg.promotion] || msg.promotion;
  }

  const result = room.chess.move(moveObj);
  if (!result) {
    send(ws, { type: 'invalid_move', reason: 'Illegal move' });
    return;
  }

  room.lastActivity = Date.now();
  room.moveHistory.push({
    from: msg.from,
    to: msg.to,
    promotion: msg.promotion || '',
    color: playerColor
  });

  const newTurn = room.chess.turn() === 'w' ? 'white' : 'black';

  // Check game over
  if (room.chess.isCheckmate()) {
    room.status = 'finished';
    room.broadcast({
      type: 'move',
      from: msg.from,
      to: msg.to,
      promotion: msg.promotion || '',
      fen: room.chess.fen(),
      moveHistory: room.moveHistory,
      currentTurn: newTurn
    });
    room.broadcast({
      type: 'game_over',
      status: 'checkmate',
      winner: playerColor
    });
    console.log(`Room ${conn.roomCode}: checkmate, ${playerColor} wins`);
    scheduleRoomCleanup(conn.roomCode, 60000);
    return;
  }

  if (room.chess.isDraw() || room.chess.isStalemate()) {
    room.status = 'finished';
    room.broadcast({
      type: 'move',
      from: msg.from,
      to: msg.to,
      promotion: msg.promotion || '',
      fen: room.chess.fen(),
      moveHistory: room.moveHistory,
      currentTurn: newTurn
    });
    room.broadcast({
      type: 'game_over',
      status: 'draw',
      winner: null
    });
    console.log(`Room ${conn.roomCode}: draw`);
    scheduleRoomCleanup(conn.roomCode, 60000);
    return;
  }

  // Normal move
  room.broadcast({
    type: 'move',
    from: msg.from,
    to: msg.to,
    promotion: msg.promotion || '',
    fen: room.chess.fen(),
    moveHistory: room.moveHistory,
    currentTurn: newTurn
  });
}

function handleResign(ws) {
  const conn = connections.get(ws);
  if (!conn) return;

  const room = rooms.get(conn.roomCode);
  if (!room || room.status !== 'active') return;

  const resignColor = room.getColorForRole(conn.role);
  const winnerColor = resignColor === 'white' ? 'black' : 'white';
  room.status = 'finished';

  room.broadcast({
    type: 'game_over',
    status: 'resignation',
    winner: winnerColor
  });
  console.log(`Room ${conn.roomCode}: ${resignColor} resigns, ${winnerColor} wins`);
  scheduleRoomCleanup(conn.roomCode, 60000);
}

function handleDisconnect(ws) {
  const conn = connections.get(ws);
  if (!conn) return;

  const room = rooms.get(conn.roomCode);
  if (!room) {
    connections.delete(ws);
    return;
  }

  const role = conn.role;
  const player = room.getPlayer(role);
  if (player) {
    player.connected = false;
  }

  if (room.status === 'waiting') {
    // Host disconnected while waiting — remove room
    rooms.delete(conn.roomCode);
    connections.delete(ws);
    console.log(`Room ${conn.roomCode} removed (host left while waiting)`);
    return;
  }

  if (room.status === 'active') {
    const opponentRole = room.getOpponentRole(role);
    room.sendTo(opponentRole, { type: 'opponent_disconnected' });

    // Start 60-second disconnect timer
    const timer = setTimeout(() => {
      if (room.status !== 'active') return;
      const opponentColor = room.getColorForRole(opponentRole);
      room.status = 'finished';
      room.sendTo(opponentRole, {
        type: 'game_over',
        status: 'abandonment',
        winner: room.getColorForRole(opponentRole)
      });
      console.log(`Room ${conn.roomCode}: ${role} timed out, ${opponentRole} wins`);
      scheduleRoomCleanup(conn.roomCode, 60000);
    }, 60000);

    room.disconnectTimers.set(role, timer);
    console.log(`${role} disconnected from room ${conn.roomCode}, 60s timer started`);
  }

  connections.delete(ws);
}

function cleanupConnection(ws) {
  const conn = connections.get(ws);
  if (conn) {
    // Don't destroy the room, just remove the ws mapping
    connections.delete(ws);
  }
}

function scheduleRoomCleanup(code, delay) {
  setTimeout(() => {
    const room = rooms.get(code);
    if (room && room.status === 'finished') {
      rooms.delete(code);
      console.log(`Room ${code} cleaned up`);
    }
  }, delay);
}

// Periodic cleanup of stale rooms (idle > 2 hours)
setInterval(() => {
  const now = Date.now();
  for (const [code, room] of rooms) {
    if (now - room.lastActivity > 2 * 60 * 60 * 1000) {
      room.broadcast({ type: 'error', message: 'Room expired due to inactivity' });
      rooms.delete(code);
      console.log(`Room ${code} expired (idle > 2h)`);
    }
  }
}, 5 * 60 * 1000); // Check every 5 minutes

// Start server
const wss = new WebSocketServer({ port: PORT });

wss.on('connection', (ws) => {
  console.log('Client connected');

  ws.on('message', (data) => {
    handleMessage(ws, data.toString());
  });

  ws.on('close', () => {
    console.log('Client disconnected');
    handleDisconnect(ws);
  });

  ws.on('error', (err) => {
    console.error('WebSocket error:', err.message);
  });

  // Keepalive via ping/pong at WebSocket protocol level
  ws.isAlive = true;
  ws.on('pong', () => { ws.isAlive = true; });
});

// Server-side heartbeat: detect dead connections
const heartbeat = setInterval(() => {
  wss.clients.forEach((ws) => {
    if (!ws.isAlive) {
      console.log('Terminating unresponsive client');
      return ws.terminate();
    }
    ws.isAlive = false;
    ws.ping();
  });
}, 30000);

wss.on('close', () => {
  clearInterval(heartbeat);
});

console.log(`Chess WebSocket server running on port ${PORT}`);
console.log(`Rooms active: ${rooms.size}`);
