#pragma once
#include <vector>
#include <array>
#include "Card.h"

// 德州撲克手牌類型枚舉（由小到大排序）
enum class HandRank {
    HIGH_CARD = 0,      // 高牌
    ONE_PAIR = 1,       // 一對
    TWO_PAIR = 2,       // 兩對
    THREE_OF_KIND = 3,  // 三條
    STRAIGHT = 4,       // 順子
    FLUSH = 5,          // 同花
    FULL_HOUSE = 6,     // 葫蘆
    FOUR_OF_KIND = 7,   // 四條
    STRAIGHT_FLUSH = 8, // 同花順
    ROYAL_FLUSH = 9     // 皇家同花順
};

// 手牌評估結果
struct HandValue {
    HandRank rank;                    // 牌型
    std::array<int, 5> kickers;      // 決定勝負的牌（由大到小）
    int score;                       // 總分數（用於快速比較）
    
    HandValue() : rank(HandRank::HIGH_CARD), kickers{0}, score(0) {}
    
    // 比較運算符
    bool operator>(const HandValue& other) const;
    bool operator<(const HandValue& other) const;
    bool operator==(const HandValue& other) const;
    
    // 轉換為字串
    std::string toString() const;
};

// 手牌評估器
class HandEvaluator {
public:
    // 評估7張牌（2張底牌 + 5張公共牌）中的最佳5張牌組合
    static HandValue evaluateHand(const std::vector<Card>& cards);
    
    // 比較兩個手牌的大小
    static int compareHands(const HandValue& hand1, const HandValue& hand2);
    
    // 從7張牌中找出最佳的5張牌組合
    static std::vector<Card> getBestFiveCards(const std::vector<Card>& cards);
    
    // 工具方法
    static std::string handRankToString(HandRank rank);
    static int getCardValue(const Card& card);
    
private:
    // 檢查各種牌型
    static HandValue checkRoyalFlush(const std::vector<Card>& cards);
    static HandValue checkStraightFlush(const std::vector<Card>& cards);
    static HandValue checkFourOfKind(const std::vector<Card>& cards);
    static HandValue checkFullHouse(const std::vector<Card>& cards);
    static HandValue checkFlush(const std::vector<Card>& cards);
    static HandValue checkStraight(const std::vector<Card>& cards);
    static HandValue checkThreeOfKind(const std::vector<Card>& cards);
    static HandValue checkTwoPair(const std::vector<Card>& cards);
    static HandValue checkOnePair(const std::vector<Card>& cards);
    static HandValue checkHighCard(const std::vector<Card>& cards);
    
    // 輔助函數
    static int calculateScore(HandRank rank, const std::array<int, 5>& kickers);
};