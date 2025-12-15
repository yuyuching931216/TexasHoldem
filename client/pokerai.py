#!/usr/bin/env python3
"""
Texas Hold'em Poker AI Client
自動化 AI 客戶端，用於連接到德州撲克伺服器
"""

import socket
import threading
import random
import itertools
import sys
import time
import json

# ==========================
#      手牌與牌組邏輯
# ==========================
RANKS = '23456789TJQKA'
SUITS = 'hdcs'

def parse_card_json(card_data):
    """
    解析 JSON 格式的牌
    card_data: {"rank": "A", "suit": "hearts", "short": "Ah"}
    返回: (rank_char, suit_char) 如 ('A', 'h')
    """
    if isinstance(card_data, dict):
        rank = card_data.get('rank', card_data.get('short', '?')[0])
        suit = card_data.get('suit', 'hearts')[0].lower()
        
        # 處理 rank
        if rank == '10':
            rank = 'T'
        elif len(rank) > 1:
            rank = rank[0].upper()
        else:
            rank = rank.upper()
        
        return (rank, suit)
    return None

def make_deck(exclude):
    """建立排除指定牌後的牌組"""
    deck = [(r, s) for r in RANKS for s in SUITS]
    return [c for c in deck if c not in exclude]

def rank_value(card):
    """獲取牌面數值"""
    return RANKS.index(card[0])

def evaluate_5(cards):
    """評估 5 張牌的牌型強度"""
    if len(cards) != 5:
        return 0
    
    ranks = sorted([rank_value(c) for c in cards], reverse=True)
    counts = {r: ranks.count(r) for r in set(ranks)}
    flush = len({s for (_, s) in cards}) == 1
    
    # 檢查順子
    unique_ranks = sorted(set(ranks))
    straight = (len(unique_ranks) == 5 and max(ranks) - min(ranks) == 4)
    # A-2-3-4-5 順子
    if set(ranks) == {12, 0, 1, 2, 3}:
        straight = True

    if straight and flush: return 8  # 同花順
    if max(counts.values()) == 4: return 7  # 四條
    if max(counts.values()) == 3 and 2 in counts.values(): return 6  # 葫蘆
    if flush: return 5  # 同花
    if straight: return 4  # 順子
    if max(counts.values()) == 3: return 3  # 三條
    if list(counts.values()).count(2) == 2: return 2.5  # 兩對
    if max(counts.values()) == 2: return 2  # 一對
    return 1 + ranks[0]/13.0  # 高牌

def best_of_7(cards):
    """從 7 張牌中選出最佳 5 張"""
    if len(cards) < 5:
        return 0
    return max(evaluate_5(list(c)) for c in itertools.combinations(cards, 5))

def estimate_equity(hole, board, num_opponents=1, sims=300):
    """蒙特卡洛模擬估算勝率"""
    if not hole:
        return 0.5
    
    used = hole + board
    deck = make_deck(used)
    wins = ties = 0

    for _ in range(sims):
        random.shuffle(deck)
        need = 5 - len(board)
        sim_board = board + deck[:need]
        index = need
        
        opponents = []
        for _ in range(num_opponents):
            if index + 1 < len(deck):
                opponents.append([deck[index], deck[index+1]])
                index += 2
        
        if not opponents:
            continue
            
        my_best = best_of_7(hole + sim_board)
        opp_best_list = [best_of_7(opp + sim_board) for opp in opponents]
        
        if all(my_best > r for r in opp_best_list):
            wins += 1
        elif any(my_best == r for r in opp_best_list):
            ties += 1
    
    if sims == 0:
        return 0.5
    return wins / sims + ties / sims * 0.5

# ==========================
#      AI 決策邏輯
# ==========================
class AIState:
    """追蹤 AI 狀態，避免無限加注"""
    def __init__(self):
        selfRaises_this_round = 0
        self.max_raises_per_round = 3  # 每回合最多加注 3 次
        self.last_stage = ""
    
    def reset_for_new_stage(self, stage):
        if stage != self.last_stage:
            self.raises_this_round = 0
            self.last_stage = stage
    
    def can_raise(self):
        return self.raises_this_round < self.max_raises_per_round
    
    def did_raise(self):
        self.raises_this_round += 1

# 全局狀態
ai_state = AIState()

def decide_action(hole, board, pot, current_bet, to_call, num_opponents, stage, my_chips=1000):
    """
    根據當前狀態決定動作
    返回: (action, amount)
    """
    global ai_state
    ai_state.reset_for_new_stage(stage)
    
    # 如果沒有籌碼，只能 check 或 fold
    if my_chips <= 0:
        print(f"[AI] No chips left! Must check or fold")
        if to_call == 0:
            return ("CHECK", 0)
        else:
            return ("FOLD", 0)
    
    # 如果籌碼不足以跟注
    if to_call > 0 and my_chips < to_call:
        print(f"[AI] Not enough chips to call (need ${to_call}, have ${my_chips})")
        # 計算 equity 決定是否 all-in
        equity = estimate_equity(hole, board, max(1, num_opponents), sims=200)
        print(f"[AI] Equity: {equity:.2f} - deciding whether to ALL-IN or FOLD")
        
        # 高 equity 時考慮 all-in
        if equity >= 0.55:
            print(f"[AI] Good equity, going ALL-IN with ${my_chips}")
            return ("ALL_IN", 0)
        else:
            print(f"[AI] Low equity, FOLD")
            return ("FOLD", 0)
    
    equity = estimate_equity(hole, board, max(1, num_opponents), sims=300)
    
    # 基礎門檻（根據階段調整）
    thresholds = {
        "preflop": (0.70, 0.45),
        "flop": (0.60, 0.35),
        "turn": (0.55, 0.30),
        "river": (0.55, 0.28)
    }
    raise_threshold, call_threshold = thresholds.get(stage, (0.55, 0.35))

    # 根據玩家數調整門檻
    raise_threshold += (num_opponents - 1) * 0.05
    call_threshold += (num_opponents - 1) * 0.03
    
    # 如果需要跟注的金額太大，提高門檻
    if to_call > 0:
        pot_odds = to_call / (pot + to_call) if (pot + to_call) > 0 else 0.5
        if equity < pot_odds * 1.2:
            call_threshold = max(call_threshold, pot_odds * 1.3)

    # Bluff 機率（降低）
    bluff_probs = {"preflop": 0.01, "flop": 0.05, "turn": 0.08, "river": 0.10}
    bluff_chance = bluff_probs.get(stage, 0.03)
    
    print(f"[AI] Equity: {equity:.2f}, Stage: {stage}, To Call: ${to_call}, Pot: ${pot}, Chips: ${my_chips}")
    print(f"[AI] Thresholds - Raise: {raise_threshold:.2f}, Call: {call_threshold:.2f}")
    
    # 計算最小加注金額
    min_raise = current_bet + 30
    need_to_raise = min_raise - (current_bet - to_call) if current_bet > 0 else min_raise
    
    # 決策邏輯
    if equity >= raise_threshold and ai_state.can_raise():
        # 計算加注金額
        raise_amount = min(current_bet * 2 + 30, my_chips // 2)
        raise_amount = max(raise_amount, min_raise)
        
        # 計算需要支付的金額
        need_to_pay = raise_amount - (current_bet - to_call) if current_bet > 0 else raise_amount
        
        if need_to_pay > my_chips:
            # 籌碼不足以加注
            if equity >= 0.70 and my_chips > to_call:
                # 高 equity，考慮 all-in
                print(f"[AI] Not enough to raise, going ALL_IN")
                ai_state.did_raise()
                return ("ALL_IN", 0)
            elif to_call <= my_chips:
                # 跟注
                print(f"[AI] Not enough to raise, CALL instead")
                return ("CALL", 0)
            elif equity >= 0.60:
                # 必須 all-in 才能繼續
                print(f"[AI] Must ALL_IN to continue")
                return ("ALL_IN", 0)
            else:
                print(f"[AI] Not enough chips and low equity, FOLD")
                return ("FOLD", 0)
        else:
            ai_state.did_raise()
            return ("RAISE", raise_amount)
            
    elif equity >= call_threshold:
        if to_call == 0:
            return ("CHECK", 0)
        elif to_call <= my_chips:
            return ("CALL", 0)
        else:
            # 籌碼不足以跟注，需要 all-in 或 fold
            if equity >= 0.50:
                print(f"[AI] Not enough to call, ALL_IN")
                return ("ALL_IN", 0)
            else:
                print(f"[AI] Not enough to call and low equity, FOLD")
                return ("FOLD", 0)
    else:
        # 考慮 bluff（只在可以加注且有籌碼時）
        if random.random() < bluff_chance and ai_state.can_raise() and to_call < pot * 0.3 and my_chips > to_call + 60:
            raise_amount = current_bet + 60
            if raise_amount <= my_chips + (current_bet - to_call):
                ai_state.did_raise()
                return ("RAISE", raise_amount)
        
        # 可以免費看牌就 check
        if to_call == 0:
            return ("CHECK", 0)
        else:
            return ("FOLD", 0)

# ==========================
#      AI 客戶端
# ==========================
class PokerAIClient:
    def __init__(self, host='localhost', port=8888, name='AI_Bot'):
        self.host = host
        self.port = port
        self.name = name
        self.sock = None
        self.running = False
        self.player_id = None
        self.my_hole = []  # [(rank, suit), ...]
        self.community_cards = []
        self.pot = 0
        self.current_bet = 0
        self.to_call = 0
        self.my_chips = 1000
        self.num_players = 0
        self.stage = "waiting"
        self.is_my_turn = False
        
    def connect(self):
        """連接到伺服器"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            self.running = True
            print(f"[AI] Connected to {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"[AI] Connection failed: {e}")
            return False

    def send(self, message):
        """發送消息"""
        try:
            self.sock.send((message + "\n").encode('utf-8'))
            print(f"[AI] Sent: {message}")
            return True
        except Exception as e:
            print(f"[AI] Send error: {e}")
            self.running = False
            return False

    def send_json(self, data):
        """發送 JSON"""
        return self.send(json.dumps(data))

    def receive_loop(self):
        """接收消息的線程"""
        buffer = ""
        while self.running:
            try:
                data = self.sock.recv(4096).decode('utf-8')
                if not data:
                    print("[AI] Server closed connection")
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
                    print(f"[AI] Receive error: {e}")
                self.running = False
                break

    def handle_message(self, message):
        """處理消息"""
        print(f"[AI] Received: {message[:100]}...")
        
        try:
            data = json.loads(message)
            self.handle_json_message(data)
        except json.JSONDecodeError:
            # 舊格式
            print(f"[AI] Non-JSON message: {message}")

    def handle_json_message(self, data):
        """處理 JSON 消息"""
        msg_type = data.get('type', '')
        
        if msg_type == 'OK':
            print(f"[AI] {data.get('message', 'OK')}")
            
        elif msg_type == 'ERROR':
            print(f"[AI] Error: {data.get('message')}")
            
        elif msg_type == 'JOINED':
            name = data.get('name')
            if name == self.name:
                self.player_id = data.get('player_id')
                self.my_chips = data.get('chips', 1000)
                print(f"[AI] I joined as ID {self.player_id} with ${self.my_chips}")
            self.num_players += 1
            
        elif msg_type == 'LEFT':
            self.num_players = max(0, self.num_players - 1)
            
        elif msg_type == 'PLAYERS':
            players = data.get('players', [])
            self.num_players = len(players)
            # 更新自己的籌碼
            for p in players:
                if p.get('id') == self.player_id:
                    self.my_chips = p.get('chips', self.my_chips)
            
        elif msg_type == 'AUTO_START_COUNTDOWN':
            seconds = data.get('seconds', 3)
            player_count = data.get('player_count', 0)
            print(f"[AI] Game starting in {seconds}s ({player_count} players)")
            
        elif msg_type == 'AUTO_START_CANCELLED':
            print(f"[AI] Auto-start cancelled")
            
        elif msg_type == 'NEW_HAND':
            print(f"[AI] New hand starting...")
            global ai_state
            ai_state = AIState()  # 重置狀態
            
        elif msg_type == 'GAME_START':
            self.stage = data.get('stage', 'preflop')
            self.my_hole = []
            self.community_cards = []
            print(f"[AI] Game started! Stage: {self.stage}")
            
        elif msg_type == 'HOLE_CARDS':
            # 收到自己的手牌！
            cards = data.get('cards', [])
            self.my_hole = [parse_card_json(c) for c in cards if parse_card_json(c)]
            print(f"[AI] My hand: {self.my_hole}")
            
        elif msg_type == 'GAME_STATE':
            self.pot = data.get('pot', 0)
            self.current_bet = data.get('current_bet', 0)
            self.stage = data.get('stage', 'waiting')
            
            # 解析公共牌
            cc = data.get('community_cards', [])
            self.community_cards = [parse_card_json(c) for c in cc if parse_card_json(c)]
            
            # 統計活躍玩家並更新自己的籌碼
            players = data.get('players', [])
            active = sum(1 for p in players if p.get('state') in [0, 2])
            self.num_players = max(2, active)
            
            for p in players:
                if p.get('id') == self.player_id:
                    self.my_chips = p.get('chips', self.my_chips)
            
        elif msg_type == 'YOUR_TURN':
            # 輪到我了！
            self.is_my_turn = True
            self.to_call = data.get('to_call', 0)
            self.pot = data.get('pot', self.pot)
            self.current_bet = data.get('current_bet', self.current_bet)
            
            print(f"[AI] My turn! To call: ${self.to_call}, My chips: ${self.my_chips}")
            
            # 做決策
            self.make_decision()
            
        elif msg_type == 'CURRENT_PLAYER':
            player_id = data.get('player_id')
            if player_id != self.player_id:
                self.is_my_turn = False
                
        elif msg_type == 'STAGE_CHANGE':
            self.stage = data.get('stage', 'unknown')
            print(f"[AI] Stage: {self.stage}")
            
        elif msg_type == 'SHOWDOWN':
            print("[AI] Showdown!")
            self.is_my_turn = False
            
        elif msg_type == 'WINNER':
            winners = data.get('winners', [])
            if not winners and data.get('player_id'):
                # 單一獲勝者格式
                print(f"[AI] Winner: {data.get('name')} wins ${data.get('amount')}")
            else:
                for w in winners:
                    print(f"[AI] Winner: {w.get('name')} wins ${w.get('amount')}")
            
        elif msg_type == 'GAME_END':
            print(f"[AI] Game ended")
            self.my_hole = []
            self.community_cards = []
            self.is_my_turn = False
            
        elif msg_type == 'BYE':
            self.running = False

    def make_decision(self):
        """做出決策並發送"""
        if not self.my_hole:
            # 沒有手牌，保守行動
            if self.to_call == 0:
                self.send("ACTION CHECK")
            elif self.to_call <= 30:
                self.send("ACTION CALL")
            else:
                self.send("ACTION FOLD")
            return
        
        # 思考時間
        time.sleep(random.uniform(0.3, 0.8))
        
        action, amount = decide_action(
            hole=self.my_hole,
            board=self.community_cards,
            pot=self.pot,
            current_bet=self.current_bet,
            to_call=self.to_call,
            num_opponents=max(1, self.num_players - 1),
            stage=self.stage,
            my_chips=self.my_chips
        )
        
        if action == "FOLD":
            self.send("ACTION FOLD")
        elif action == "CHECK":
            self.send("ACTION CHECK")
        elif action == "CALL":
            self.send("ACTION CALL")
        elif action == "RAISE":
            self.send(f"ACTION RAISE {amount}")
        elif action == "ALL_IN":
            self.send("ACTION ALL_IN")
        
        self.is_my_turn = False

    def run(self):
        """運行 AI"""
        if not self.connect():
            return
        
        # 啟動接收線程
        receive_thread = threading.Thread(target=self.receive_loop, daemon=True)
        receive_thread.start()
        
        # 加入遊戲
        time.sleep(0.5)
        self.send(f"JOIN {self.name} 1000")
        
        # 主循環
        try:
            while self.running:
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\n[AI] Interrupted")
        finally:
            self.send("QUIT")
            self.running = False

# ==========================
#      啟動 AI
# ==========================
def main():
    host = 'localhost'
    port = 8888
    name = 'AI_Bot'
    
    if len(sys.argv) > 1:
        host = sys.argv[1]
    if len(sys.argv) > 2:
        port = int(sys.argv[2])
    if len(sys.argv) > 3:
        name = sys.argv[3]
    
    print(f"[AI] Starting: {name} -> {host}:{port}")
    
    ai = PokerAIClient(host=host, port=port, name=name)
    ai.run()

if __name__ == "__main__":
    main()
