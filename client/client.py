#!/usr/bin/env python3
"""
Texas Hold'em Poker Client
支持 JSON 格式的命令行客戶端
"""

import socket
import threading
import sys
import time
import json

class PokerClient:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.player_id = None
        self.player_name = None
        self.my_hand = []
        self.community_cards = []
        self.pot = 0
        self.current_bet = 0
        self.is_my_turn = False
        
    def connect(self):
        """連接到伺服器"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            print(f"Connected to server at {self.host}:{self.port}")
            self.running = True
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self):
        """斷開連接"""
        self.running = False
        if self.socket:
            try:
                self.send_command("QUIT")
                self.socket.close()
            except:
                pass
        print("Disconnected from server")
    
    def send_message(self, message):
        """發送原始消息到伺服器"""
        try:
            self.socket.send((message + "\n").encode('utf-8'))
            return True
        except Exception as e:
            print(f"Send error: {e}")
            self.running = False
            return False
    
    def send_command(self, command):
        """發送文本命令"""
        return self.send_message(command)
    
    def send_json(self, data):
        """發送 JSON 格式消息"""
        return self.send_message(json.dumps(data))
    
    def receive_loop(self):
        """接收消息的線程"""
        buffer = ""
        while self.running:
            try:
                data = self.socket.recv(4096).decode('utf-8')
                if not data:
                    print("Server closed connection")
                    self.running = False
                    break
                
                buffer += data
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()
                    if line:
                        self.handle_message(line)
                        
            except Exception as e:
                if self.running:
                    print(f"Receive error: {e}")
                self.running = False
                break
    
    def handle_message(self, message):
        """處理從伺服器接收的消息"""
        # 嘗試解析為 JSON
        try:
            data = json.loads(message)
            self.handle_json_message(data)
        except json.JSONDecodeError:
            # 舊格式的文本消息
            self.handle_text_message(message)
    
    def handle_json_message(self, data):
        """處理 JSON 格式的消息"""
        msg_type = data.get('type', '')
        
        if msg_type == 'OK':
            print(f"✓ {data.get('message', 'OK')}")
            
        elif msg_type == 'ERROR':
            print(f"✗ Error: {data.get('message', 'Unknown error')}")
            
        elif msg_type == 'JOINED':
            player_id = data.get('player_id')
            name = data.get('name')
            chips = data.get('chips')
            print(f"→ Player {name} (ID: {player_id}) joined with ${chips}")
            
        elif msg_type == 'LEFT':
            player_id = data.get('player_id')
            print(f"← Player {player_id} left the game")
            
        elif msg_type == 'PLAYERS':
            print("\n=== Current Players ===")
            for player in data.get('players', []):
                print(f"  ID: {player['id']}, Name: {player['name']}, Chips: ${player['chips']}")
            print()
            
        elif msg_type == 'AUTO_START_COUNTDOWN':
            seconds = data.get('seconds', 3)
            player_count = data.get('player_count', 0)
            message = data.get('message', '')
            print(f"\n⏱️  {message}")
            print(f"   Players ready: {player_count}")
            
        elif msg_type == 'AUTO_START_CANCELLED':
            message = data.get('message', 'Auto-start cancelled')
            print(f"\n❌ {message}")
            
        elif msg_type == 'GAME_START':
            print(f"\n🎮 {data.get('message', 'Game starting...')}")
            print(f"Stage: {data.get('stage', 'unknown')}")
            
        elif msg_type == 'HOLE_CARDS':
            self.my_hand = data.get('cards', [])
            print("\n🃏 Your hole cards:")
            for card in self.my_hand:
                print(f"  {card.get('rank')} of {card.get('suit')}")
            print()
            
        elif msg_type == 'GAME_STATE':
            self.display_game_state_json(data)
            
        elif msg_type == 'ROOM_STATE':
            self.display_room_state_json(data)
            
        elif msg_type == 'YOUR_TURN':
            self.is_my_turn = True
            to_call = data.get('to_call', 0)
            pot = data.get('pot', 0)
            current_bet = data.get('current_bet', 0)
            min_raise = data.get('min_raise', 0)
            
            print("\n" + "="*50)
            print("🎯 IT'S YOUR TURN!")
            print(f"   Pot: ${pot}")
            print(f"   Current bet: ${current_bet}")
            print(f"   To call: ${to_call}")
            print(f"   Min raise: ${min_raise}")
            print("="*50)
            print("Commands: fold, check, call, raise <amount>, allin")
            print()
            
        elif msg_type == 'CURRENT_PLAYER':
            player_id = data.get('player_id')
            print(f"⏳ Waiting for player {player_id}...")
            
        elif msg_type == 'ACTION':
            player_id = data.get('player_id')
            action = data.get('action')
            amount = data.get('amount', '')
            if amount:
                print(f"→ Player {player_id}: {action} ${amount}")
            else:
                print(f"→ Player {player_id}: {action}")
                
        elif msg_type == 'STAGE_CHANGE':
            stage = data.get('stage', 'unknown')
            print(f"\n📍 Stage: {stage.upper()}")
            
        elif msg_type == 'SHOWDOWN':
            print("\n" + "="*50)
            print("🏆 SHOWDOWN!")
            for player in data.get('players', []):
                name = player.get('name')
                chips = player.get('chips')
                hand = player.get('hand', [])
                hand_str = ', '.join([f"{c['rank']}{c['suit'][0]}" for c in hand])
                print(f"  {name} (${chips}): [{hand_str}]")
            print("="*50 + "\n")
            
        elif msg_type == 'GAME_END':
            print(f"\n🏁 {data.get('message', 'Game ended')}")
            self.my_hand = []
            self.is_my_turn = False
            
        elif msg_type == 'BYE':
            print("Goodbye!")
            self.running = False
            
        else:
            print(f"Server: {json.dumps(data, indent=2)}")
    
    def handle_text_message(self, message):
        """處理舊格式的文本消息（向後兼容）"""
        parts = message.split('|')
        command = parts[0]
        
        if command == "OK":
            print(f"✓ {parts[1] if len(parts) > 1 else 'OK'}")
        elif command == "ERROR":
            print(f"✗ Error: {parts[1] if len(parts) > 1 else 'Unknown error'}")
        else:
            print(f"Server: {message}")
    
    def display_game_state_json(self, data):
        """顯示 JSON 格式的遊戲狀態"""
        pot = data.get('pot', 0)
        current_bet = data.get('current_bet', 0)
        stage = data.get('stage', 'waiting')
        community_cards = data.get('community_cards', [])
        players = data.get('players', [])
        current_player = data.get('current_player', -1)
        
        print("\n" + "="*50)
        print(f"📍 Stage: {stage.upper()}")
        print(f"💰 Pot: ${pot} | Current Bet: ${current_bet}")
        
        # 顯示公共牌
        if community_cards:
            cards_str = ', '.join([f"{c['rank']}{c['suit'][0]}" for c in community_cards])
            print(f"🃏 Community Cards: [{cards_str}]")
        
        # 顯示自己的手牌
        if self.my_hand:
            hand_str = ', '.join([f"{c['rank']}{c['suit'][0]}" for c in self.my_hand])
            print(f"🎴 Your Hand: [{hand_str}]")
        
        # 顯示玩家信息
        print("\n👥 Players:")
        for player in players:
            p_id = player.get('id')
            name = player.get('name')
            chips = player.get('chips')
            state = player.get('state', 0)
            bet = player.get('current_bet', 0)
            is_dealer = player.get('is_dealer', False)
            is_sb = player.get('is_small_blind', False)
            is_bb = player.get('is_big_blind', False)
            
            state_str = self.get_state_string(state)
            position = ""
            if is_dealer: position += "🎲"
            if is_sb: position += "SB"
            if is_bb: position += "BB"
            
            turn_indicator = " ← " if p_id == current_player else "   "
            
            print(f"  {turn_indicator}{name} (${chips}) [{state_str}] Bet: ${bet} {position}")
        
        print("="*50 + "\n")
    
    def display_room_state_json(self, data):
        """顯示房間狀態"""
        room_id = data.get('room_id', 0)
        player_count = data.get('player_count', 0)
        max_players = data.get('max_players', 10)
        in_progress = data.get('game_in_progress', False)
        stage = data.get('stage', 'waiting')
        
        print(f"📊 Room {room_id} - Players: {player_count}/{max_players}")
        print(f"   Game: {'In Progress' if in_progress else 'Waiting'}")
        print(f"   Stage: {stage}")
    
    def get_state_string(self, state_code):
        """將狀態代碼轉換為字串"""
        states = {
            0: "Active",
            1: "Folded",
            2: "All-in",
            3: "Disconnected",
            4: "Waiting"
        }
        return states.get(state_code, "Unknown")
    
    def interactive_mode(self):
        """互動模式"""
        print("\n=== Texas Hold'em Poker Client (JSON) ===")
        print("Commands:")
        print("  join <name> [buyin]  - Join the game")
        print("  start                - Start the game")
        print("  fold                 - Fold your hand")
        print("  check                - Check")
        print("  call                 - Call current bet")
        print("  raise <amount>       - Raise bet")
        print("  allin                - Go all-in")
        print("  status               - Get room status")
        print("  gamestate            - Get detailed game state")
        print("  players              - Get player list")
        print("  quit                 - Leave the game")
        print("  help                 - Show this help")
        print()
        
        # 啟動接收線程
        receive_thread = threading.Thread(target=self.receive_loop, daemon=True)
        receive_thread.start()
        
        # 主命令循環
        while self.running:
            try:
                user_input = input("> ").strip()
                
                if not user_input:
                    continue
                
                parts = user_input.split()
                command = parts[0].lower()
                
                if command == "help":
                    print("\nCommands:")
                    print("  join <name> [buyin]  - Join the game")
                    print("  start                - Start the game")
                    print("  fold                 - Fold your hand")
                    print("  check                - Check")
                    print("  call                 - Call current bet")
                    print("  raise <amount>       - Raise bet")
                    print("  allin                - Go all-in")
                    print("  status               - Get room status")
                    print("  gamestate            - Get detailed game state")
                    print("  players              - Get player list")
                    print("  quit                 - Leave the game\n")
                    
                elif command == "join":
                    if len(parts) < 2:
                        print("Usage: join <name> [buyin]")
                        continue
                    name = parts[1]
                    buyin = parts[2] if len(parts) > 2 else "1000"
                    self.player_name = name
                    self.send_command(f"JOIN {name} {buyin}")
                    
                elif command == "start":
                    self.send_command("START")
                    
                elif command in ["fold", "check", "call"]:
                    self.send_command(f"ACTION {command.upper()}")
                    self.is_my_turn = False
                    
                elif command == "raise":
                    if len(parts) < 2:
                        print("Usage: raise <amount>")
                        continue
                    amount = parts[1]
                    self.send_command(f"ACTION RAISE {amount}")
                    self.is_my_turn = False
                    
                elif command == "allin":
                    self.send_command("ACTION ALL_IN")
                    self.is_my_turn = False
                    
                elif command == "status":
                    self.send_command("STATUS")
                    
                elif command == "gamestate":
                    self.send_command("GAMESTATE")
                    
                elif command == "players":
                    self.send_command("PLAYERS")
                    
                elif command == "quit":
                    self.disconnect()
                    break
                    
                else:
                    print(f"Unknown command: {command}. Type 'help' for help.")
                    
            except KeyboardInterrupt:
                print("\nInterrupted by user")
                self.disconnect()
                break
            except EOFError:
                self.disconnect()
                break
            except Exception as e:
                print(f"Error: {e}")

def main():
    host = 'localhost'
    port = 8888
    
    # 解析命令行參數
    if len(sys.argv) > 1:
        host = sys.argv[1]
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    
    client = PokerClient(host, port)
    
    if client.connect():
        try:
            client.interactive_mode()
        finally:
            client.disconnect()
    else:
        print("Failed to connect to server")
        sys.exit(1)

if __name__ == "__main__":
    main()
