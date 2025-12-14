import socket
import json
import random
import itertools

# ==========================
#      手牌與牌組邏輯
# ==========================
RANKS = '23456789TJQKA'
SUITS = 'cdhs'

def parse_card(s):
    return s[0].upper(), s[1].lower()

def make_deck(exclude):
    deck = [(r, s) for r in RANKS for s in SUITS]
    return [c for c in deck if c not in exclude]

def rank_value(card):
    return RANKS.index(card[0])

def evaluate_5(cards):
    ranks = sorted([rank_value(c) for c in cards], reverse=True)
    counts = {r: ranks.count(r) for r in set(ranks)}
    flush = len({s for (_, s) in cards}) == 1
    straight = (max(ranks) - min(ranks) == 4) and len(set(ranks)) == 5

    if straight and flush: return 8
    if max(counts.values()) == 4: return 7
    if max(counts.values()) == 3 and 2 in counts.values(): return 6
    if flush: return 5
    if straight: return 4
    if max(counts.values()) == 3: return 3
    if list(counts.values()).count(2) == 2: return 2.5
    if max(counts.values()) == 2: return 2
    return 1 + ranks[0]/13.0

def best_of_7(cards):
    return max(evaluate_5(list(c)) for c in itertools.combinations(cards, 5))

def estimate_equity(hole, board, num_opponents=1, sims=1000):
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
            opponents.append([deck[index], deck[index+1]])
            index += 2
        my_best = best_of_7(hole + sim_board)
        opp_best_list = [best_of_7(opp + sim_board) for opp in opponents]
        if all(my_best > r for r in opp_best_list):
            wins += 1
        elif any(my_best == r for r in opp_best_list):
            ties += 1
    return wins / sims + ties / sims * 0.5

# ==========================
#      AI 決策（Stage-aware + Bluff）
# ==========================
def decide_action(hole, board, pot, current_bet, num_opponents, stage):
    equity = estimate_equity(hole, board, num_opponents, sims=800)

    # 基礎門檻
    thresholds = {
        "preflop": (0.6, 0.4),
        "flop": (0.55, 0.35),
        "turn": (0.5, 0.3),
        "river": (0.5, 0.25)
    }
    raise_threshold, call_threshold = thresholds.get(stage, (0.55, 0.35))

    # 根據玩家數調整門檻
    raise_threshold += (num_opponents - 1) * 0.05
    call_threshold += (num_opponents - 1) * 0.03

    # bluff 機率
    bluff_probs = {"preflop": 0.02, "flop": 0.1, "turn": 0.15, "river": 0.2}
    bluff_factor = min(pot / 200, 0.25)
    bluff_chance = bluff_probs.get(stage) + bluff_factor
    

    # 正常決策
    if equity >= raise_threshold:
        return {"action": "RAISE", "amount": max(current_bet*2, 10)}
    elif equity >= call_threshold:
        return {"action": "CALL"}
    else:
        if random.random() < bluff_chance:
            return {"action": "RAISE", "amount": max(current_bet*2, 10)}
        return {"action": "FOLD"}

# ==========================
#      AI 客戶端
# ==========================
class PokerAIClient:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.sock = None
        self.running = False

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        self.running = True
        print(f"[AI] Connected to {self.host}:{self.port}")

    def send(self, msg_dict):
        msg = json.dumps(msg_dict) + "\n"
        self.sock.send(msg.encode())

    def receive(self):
        data = ""
        while "\n" not in data:
            chunk = self.sock.recv(1024).decode()
            if not chunk:
                self.running = False
                break
            data += chunk
        return json.loads(data.strip())

    def run(self):
        while self.running:
            try:
                state = self.receive()
                if "hole" in state and "community_cards" in state and "stage" in state:
                    action = decide_action(
                        hole=[parse_card(c) for c in state["hole"]],
                        board=[parse_card(c) for c in state["community_cards"]],
                        pot=state.get("pot", 0),
                        current_bet=state.get("current_bet", 0),
                        num_opponents=len(state.get("players", [])) - 1,
                        stage=state["stage"]
                    )
                    self.send(action)
            except Exception as e:
                print("[AI] Error:", e)
                self.running = False
                break

# ==========================
#      啟動 AI
# ==========================
if __name__ == "__main__":
    ai = PokerAIClient(host='192.168.0.100', port=8888)
    ai.connect()
    ai.run()
