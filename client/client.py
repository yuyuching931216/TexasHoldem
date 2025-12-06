#!/usr/bin/env python3
"""
Texas Hold'em Poker Client
簡單的命令行客戶端，用於連接到德州撲克伺服器
"""

import socket
import threading
import sys
import time

class PokerClient:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.socket = None
        self.running = False
        self.player_id = None
        self.player_name = None
        
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
                self.socket.send(b"QUIT\n")
                self.socket.close()
            except:
                pass
        print("Disconnected from server")
    
    def send_message(self, message):
        """發送消息到伺服器"""
        try:
            self.socket.send((message + "\n").encode('utf-8'))
            return True
        except Exception as e:
            print(f"Send error: {e}")
            self.running = False
            return False
    
    def receive_loop(self):
        """接收消息的線程"""
        buffer = ""
        while self.running:
            try:
                data = self.socket.recv(1024).decode('utf-8')
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
        parts = message.split('|')
        command = parts[0]
        
        if command == "OK":
            print(f"✓ {parts[1] if len(parts) > 1 else 'OK'}")
            
        elif command == "ERROR":
            print(f"✗ Error: {parts[1] if len(parts) > 1 else 'Unknown error'}")
            
        elif command == "JOINED":
            if len(parts) >= 4:
                player_id, name, chips = parts[1], parts[2], parts[3]
                print(f"→ Player {name} (ID: {player_id}) joined with ${chips}")
                
        elif command == "LEFT":
            if len(parts) >= 2:
                player_id = parts[1]
                print(f"← Player {player_id} left the game")
                
        elif command == "PLAYERS":
            if len(parts) >= 2:
                print("\n=== Current Players ===")
                players = parts[1].split('|')
                for player in players:
                    if player:
                        p_parts = player.split(',')
                        if len(p_parts) >= 3:
                            print(f"  ID: {p_parts[0]}, Name: {p_parts[1]}, Chips: ${p_parts[2]}")
                print()
                
        elif command == "GAME_START":
            print(f"\n🎮 {parts[1] if len(parts) > 1 else 'Game starting...'}")
            
        elif command == "STATE":
            self.display_game_state(parts[1] if len(parts) > 1 else "")
            
        elif command == "ACTION":
            if len(parts) >= 3:
                player_id, action = parts[1], parts[2]
                amount = parts[3] if len(parts) > 3 else ""
                if amount:
                    print(f"→ Player {player_id}: {action} ${amount}")
                else:
                    print(f"→ Player {player_id}: {action}")
                    
        elif command == "STATUS":
            print(f"📊 {parts[1] if len(parts) > 1 else 'Status'}")
            
        elif command == "BYE":
            print("Goodbye!")
            self.running = False
            
        else:
            print(f"Server: {message}")
    
    def display_game_state(self, state_data):
        """顯示遊戲狀態"""
        try:
            parts = state_data.split('|')
            if not parts:
                return
            
            # 解析基本信息: pot,currentBet,communityCards
            basic = parts[0].split(',')
            pot = basic[0] if len(basic) > 0 else "0"
            current_bet = basic[1] if len(basic) > 1 else "0"
            community_cards = basic[2] if len(basic) > 2 else ""
            
            print("\n" + "="*50)
            print(f"💰 Pot: ${pot} | Current Bet: ${current_bet}")
            
            if community_cards:
                cards = community_cards.split(';')
                print(f"🃏 Community Cards: {', '.join(cards)}")
            
            # 顯示玩家信息
            if len(parts) > 1:
                print("\n👥 Players:")
                for i in range(1, len(parts)):
                    player_info = parts[i].split(',')
                    if len(player_info) >= 4:
                        p_id, name, chips, state = player_info[0], player_info[1], player_info[2], player_info[3]
                        state_str = self.get_state_string(state)
                        print(f"  {name} (${chips}) [{state_str}]")
            
            print("="*50 + "\n")
            
        except Exception as e:
            print(f"Error displaying game state: {e}")
    
    def get_state_string(self, state_code):
        """將狀態代碼轉換為字串"""
        states = {
            "0": "Active",
            "1": "Folded",
            "2": "All-in",
            "3": "Disconnected",
            "4": "Waiting"
        }
        return states.get(state_code, "Unknown")
    
    def interactive_mode(self):
        """互動模式"""
        print("\n=== Texas Hold'em Poker Client ===")
        print("Commands:")
        print("  join <name> [buyin]  - Join the game")
        print("  start                - Start the game")
        print("  fold                 - Fold your hand")
        print("  check                - Check")
        print("  call                 - Call current bet")
        print("  raise <amount>       - Raise bet")
        print("  allin                - Go all-in")
        print("  status               - Get room status")
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
                    print("  quit                 - Leave the game\n")
                    
                elif command == "join":
                    if len(parts) < 2:
                        print("Usage: join <name> [buyin]")
                        continue
                    name = parts[1]
                    buyin = parts[2] if len(parts) > 2 else "1000"
                    self.player_name = name
                    self.send_message(f"JOIN {name} {buyin}")
                    
                elif command == "start":
                    self.send_message("START")
                    
                elif command in ["fold", "check", "call"]:
                    self.send_message(f"ACTION {command.upper()}")
                    
                elif command == "raise":
                    if len(parts) < 2:
                        print("Usage: raise <amount>")
                        continue
                    amount = parts[1]
                    self.send_message(f"ACTION RAISE {amount}")
                    
                elif command == "allin":
                    self.send_message("ACTION ALL_IN")
                    
                elif command == "status":
                    self.send_message("STATUS")
                    
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
