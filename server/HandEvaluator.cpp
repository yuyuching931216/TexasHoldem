#include "HandEvaluator.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש
// HandValue ¹ך²{
// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש

bool HandValue::operator>(const HandValue& other) const {
    return score > other.score;
}

bool HandValue::operator<(const HandValue& other) const {
    return score < other.score;
}

bool HandValue::operator==(const HandValue& other) const {
    return score == other.score;
}

std::string HandValue::toString() const {
    std::stringstream ss;
    ss << HandEvaluator::handRankToString(rank);
    
    // ²K¥[ֳצֱהµP¸ך°T
    ss << " (";
    for (size_t i = 0; i < kickers.size() && kickers[i] != 0; ++i) {
        if (i > 0) ss << ", ";
        // ֲא´«¼ֶ¦r¬°µP­±­ָ
        if (kickers[i] == 14) ss << "A";
        else if (kickers[i] == 13) ss << "K";
        else if (kickers[i] == 12) ss << "Q";
        else if (kickers[i] == 11) ss << "J";
        else ss << kickers[i];
    }
    ss << ")";
    
    return ss.str();
}

// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש
// HandEvaluator ¹ך²{
// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש

int HandEvaluator::getCardValue(const Card& card) {
    // ±N Rank enum ֲא´«¬°¼ֶ¦r­ָ
    return static_cast<int>(card.getRank());
}

HandValue HandEvaluator::evaluateHand(const std::vector<Card>& cards) {
    if (cards.size() < 5) {
        // ₪£¨¬5±iµP¡A×נ¦^×ֵ₪גµP
        return HandValue();
    }
    
    // «צ·׃µP«¬ְu¥‎¯ֵ¨ּ¦¸ְֻ¬d
    HandValue result;
    
    // ְֻ¬d¬׃®a¦P×ב¶¶
    result = checkRoyalFlush(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¦P×ב¶¶
    result = checkStraightFlush(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¥|±ר
    result = checkFourOfKind(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¸¬ִ×
    result = checkFullHouse(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¦P×ב
    result = checkFlush(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¶¶₪l
    result = checkStraight(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d₪T±ר
    result = checkThreeOfKind(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d¨ג¹ן
    result = checkTwoPair(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // ְֻ¬d₪@¹ן
    result = checkOnePair(cards);
    if (result.rank != HandRank::HIGH_CARD) return result;
    
    // °×µP
    return checkHighCard(cards);
}

int HandEvaluator::compareHands(const HandValue& hand1, const HandValue& hand2) {
    if (hand1.score > hand2.score) return 1;
    if (hand1.score < hand2.score) return -1;
    return 0;
}

std::vector<Card> HandEvaluator::getBestFiveCards(const std::vector<Card>& cards) {
    // ³o¬O₪@­׃ֲ²₪ֶ×©¥»¡A¹ך»Ú₪W»Ý­nְֻ¬d©ׂ¦³¥i¯א×÷5±iµP²ױ¦X
    std::vector<Card> sortedCards = cards;
    std::sort(sortedCards.begin(), sortedCards.end(), [](const Card& a, const Card& b) {
        return getCardValue(a) > getCardValue(b);
    });
    
    // ¨ת«e5±i¡]«צµP­±­ָ±ֶ§ַ¡^
    std::vector<Card> bestFive;
    for (int i = 0; i < 5 && i < static_cast<int>(sortedCards.size()); ++i) {
        bestFive.push_back(sortedCards[i]);
    }
    
    return bestFive;
}

// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש
// ¨p¦³ְֻ¬d¨ח¼ֶ¹ך²{
// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש

HandValue HandEvaluator::checkRoyalFlush(const std::vector<Card>& cards) {
    // ¬׃®a¦P×ב¶¶¡GA, K, Q, J, 10 ¦P×ב
    std::map<Suit, std::vector<Card>> suitCards;
    
    // «צ×ב¦ג₪ְ²ױ
    for (const auto& card : cards) {
        suitCards[card.getSuit()].push_back(card);
    }
    
    for (const auto& pair : suitCards) {
        if (pair.second.size() >= 5) {
            std::vector<int> values;
            for (const auto& card : pair.second) {
                values.push_back(getCardValue(card));
            }
            std::sort(values.rbegin(), values.rend());
            
            // ְֻ¬d¬O§_¦³ A, K, Q, J, 10
            if (values.size() >= 5 && values[0] == 14 && values[1] == 13 && 
                values[2] == 12 && values[3] == 11 && values[4] == 10) {
                HandValue result;
                result.rank = HandRank::ROYAL_FLUSH;
                result.kickers = {14, 13, 12, 11, 10};
                result.score = calculateScore(result.rank, result.kickers);
                return result;
            }
        }
    }
    
    return HandValue(); // ₪£¬O¬׃®a¦P×ב¶¶
}

HandValue HandEvaluator::checkStraightFlush(const std::vector<Card>& cards) {
    // ¦P×ב¶¶¡G³sִע×÷5±i¦P×בµP
    std::map<Suit, std::vector<int>> suitValues;
    
    // «צ×ב¦ג₪ְ²ױµP­ָ
    for (const auto& card : cards) {
        suitValues[card.getSuit()].push_back(getCardValue(card));
    }
    
    for (auto& pair : suitValues) {
        if (pair.second.size() >= 5) {
            std::sort(pair.second.rbegin(), pair.second.rend());
            
            // ְֻ¬d¬O§_¦³³sִע×÷5±iµP
            for (size_t i = 0; i <= pair.second.size() - 5; ++i) {
                bool isSequential = true;
                for (int j = 0; j < 4; ++j) {
                    if (pair.second[i + j] - pair.second[i + j + 1] != 1) {
                        isSequential = false;
                        break;
                    }
                }
                
                if (isSequential) {
                    HandValue result;
                    result.rank = HandRank::STRAIGHT_FLUSH;
                    result.kickers = {pair.second[i], pair.second[i+1], pair.second[i+2], 
                                    pair.second[i+3], pair.second[i+4]};
                    result.score = calculateScore(result.rank, result.kickers);
                    return result;
                }
            }
            
            // ְֻ¬d A, 2, 3, 4, 5 ¦P×ב¶¶¡]½ü½L¶¶¡^
            std::set<int> valueSet(pair.second.begin(), pair.second.end());
            if (valueSet.count(14) && valueSet.count(2) && valueSet.count(3) && 
                valueSet.count(4) && valueSet.count(5)) {
                HandValue result;
                result.rank = HandRank::STRAIGHT_FLUSH;
                result.kickers = {5, 4, 3, 2, 1}; // A ·ם§@ 1
                result.score = calculateScore(result.rank, result.kickers);
                return result;
            }
        }
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkFourOfKind(const std::vector<Card>& cards) {
    std::map<int, int> valueCounts;
    
    for (const auto& card : cards) {
        valueCounts[getCardValue(card)]++;
    }
    
    int fourKindValue = 0;
    int kicker = 0;
    
    // §ה¥|±ר
    for (const auto& pair : valueCounts) {
        if (pair.second == 4) {
            fourKindValue = pair.first;
            break;
        }
    }
    
    if (fourKindValue != 0) {
        // §ה³ּ₪j×÷½נ¸}µP
        for (const auto& pair : valueCounts) {
            if (pair.first != fourKindValue && pair.first > kicker) {
                kicker = pair.first;
            }
        }
        
        HandValue result;
        result.rank = HandRank::FOUR_OF_KIND;
        result.kickers = {fourKindValue, kicker, 0, 0, 0};
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkFullHouse(const std::vector<Card>& cards) {
    std::map<int, int> valueCounts;
    
    for (const auto& card : cards) {
        valueCounts[getCardValue(card)]++;
    }
    
    int threeKindValue = 0;
    int pairValue = 0;
    
    // §ה₪T±ר©M¹ן₪l
    for (const auto& pair : valueCounts) {
        if (pair.second >= 3 && pair.first > threeKindValue) {
            if (threeKindValue != 0) {
                pairValue = std::max(pairValue, threeKindValue);
            }
            threeKindValue = pair.first;
        } else if (pair.second >= 2 && pair.first > pairValue) {
            pairValue = pair.first;
        }
    }
    
    if (threeKindValue != 0 && pairValue != 0) {
        HandValue result;
        result.rank = HandRank::FULL_HOUSE;
        result.kickers = {threeKindValue, pairValue, 0, 0, 0};
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkFlush(const std::vector<Card>& cards) {
    std::map<Suit, std::vector<int>> suitValues;
    
    for (const auto& card : cards) {
        suitValues[card.getSuit()].push_back(getCardValue(card));
    }
    
    for (auto& pair : suitValues) {
        if (pair.second.size() >= 5) {
            std::sort(pair.second.rbegin(), pair.second.rend());
            
            HandValue result;
            result.rank = HandRank::FLUSH;
            result.kickers = {pair.second[0], pair.second[1], pair.second[2], 
                            pair.second[3], pair.second[4]};
            result.score = calculateScore(result.rank, result.kickers);
            return result;
        }
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkStraight(const std::vector<Card>& cards) {
    std::set<int> uniqueValues;
    
    for (const auto& card : cards) {
        uniqueValues.insert(getCardValue(card));
    }
    
    std::vector<int> values(uniqueValues.rbegin(), uniqueValues.rend());
    
    // ְֻ¬d³sִע×÷5±iµP
    for (size_t i = 0; i <= values.size() - 5; ++i) {
        bool isSequential = true;
        for (int j = 0; j < 4; ++j) {
            if (values[i + j] - values[i + j + 1] != 1) {
                isSequential = false;
                break;
            }
        }
        
        if (isSequential) {
            HandValue result;
            result.rank = HandRank::STRAIGHT;
            result.kickers = {values[i], values[i+1], values[i+2], values[i+3], values[i+4]};
            result.score = calculateScore(result.rank, result.kickers);
            return result;
        }
    }
    
    // ְֻ¬d A, 2, 3, 4, 5 ¶¶₪l¡]½ü½L¶¶¡^
    if (uniqueValues.count(14) && uniqueValues.count(2) && uniqueValues.count(3) && 
        uniqueValues.count(4) && uniqueValues.count(5)) {
        HandValue result;
        result.rank = HandRank::STRAIGHT;
        result.kickers = {5, 4, 3, 2, 1}; // A ·ם§@ 1
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkThreeOfKind(const std::vector<Card>& cards) {
    std::map<int, int> valueCounts;
    
    for (const auto& card : cards) {
        valueCounts[getCardValue(card)]++;
    }
    
    int threeKindValue = 0;
    std::vector<int> kickers;
    
    // §ה₪T±ר
    for (const auto& pair : valueCounts) {
        if (pair.second == 3 && pair.first > threeKindValue) {
            threeKindValue = pair.first;
        }
    }
    
    if (threeKindValue != 0) {
        // §ה½נ¸}µP
        for (const auto& pair : valueCounts) {
            if (pair.first != threeKindValue) {
                for (int i = 0; i < pair.second; ++i) {
                    kickers.push_back(pair.first);
                }
            }
        }
        std::sort(kickers.rbegin(), kickers.rend());
        
        HandValue result;
        result.rank = HandRank::THREE_OF_KIND;
        result.kickers = {threeKindValue, 
                         kickers.size() > 0 ? kickers[0] : 0,
                         kickers.size() > 1 ? kickers[1] : 0, 0, 0};
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkTwoPair(const std::vector<Card>& cards) {
    std::map<int, int> valueCounts;
    
    for (const auto& card : cards) {
        valueCounts[getCardValue(card)]++;
    }
    
    std::vector<int> pairs;
    std::vector<int> kickers;
    
    for (const auto& pair : valueCounts) {
        if (pair.second >= 2) {
            pairs.push_back(pair.first);
            if (pair.second > 2) {
                // ₪T±ר¥H₪W÷ג§@¹ן₪l¡A³ׁ¾l×÷·ם½נ¸}µP
                for (int i = 2; i < pair.second; ++i) {
                    kickers.push_back(pair.first);
                }
            }
        } else {
            for (int i = 0; i < pair.second; ++i) {
                kickers.push_back(pair.first);
            }
        }
    }
    
    if (pairs.size() >= 2) {
        std::sort(pairs.rbegin(), pairs.rend());
        std::sort(kickers.rbegin(), kickers.rend());
        
        HandValue result;
        result.rank = HandRank::TWO_PAIR;
        result.kickers = {pairs[0], pairs[1], 
                         kickers.size() > 0 ? kickers[0] : 0, 0, 0};
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkOnePair(const std::vector<Card>& cards) {
    std::map<int, int> valueCounts;
    
    for (const auto& card : cards) {
        valueCounts[getCardValue(card)]++;
    }
    
    int pairValue = 0;
    std::vector<int> kickers;
    
    for (const auto& pair : valueCounts) {
        if (pair.second >= 2 && pair.first > pairValue) {
            // ¦p×G₪§«e¦³¹ן₪l¡A­°¯ֵ¬°½נ¸}µP
            if (pairValue != 0) {
                kickers.push_back(pairValue);
            }
            pairValue = pair.first;
            
            // ¦h¾l×÷µP·ם½נ¸}µP
            for (int i = 2; i < pair.second; ++i) {
                kickers.push_back(pair.first);
            }
        } else {
            for (int i = 0; i < pair.second; ++i) {
                kickers.push_back(pair.first);
            }
        }
    }
    
    if (pairValue != 0) {
        std::sort(kickers.rbegin(), kickers.rend());
        
        HandValue result;
        result.rank = HandRank::ONE_PAIR;
        result.kickers = {pairValue, 
                         kickers.size() > 0 ? kickers[0] : 0,
                         kickers.size() > 1 ? kickers[1] : 0,
                         kickers.size() > 2 ? kickers[2] : 0, 0};
        result.score = calculateScore(result.rank, result.kickers);
        return result;
    }
    
    return HandValue();
}

HandValue HandEvaluator::checkHighCard(const std::vector<Card>& cards) {
    std::vector<int> values;
    
    for (const auto& card : cards) {
        values.push_back(getCardValue(card));
    }
    
    std::sort(values.rbegin(), values.rend());
    
    HandValue result;
    result.rank = HandRank::HIGH_CARD;
    result.kickers = {values.size() > 0 ? values[0] : 0,
                     values.size() > 1 ? values[1] : 0,
                     values.size() > 2 ? values[2] : 0,
                     values.size() > 3 ? values[3] : 0,
                     values.size() > 4 ? values[4] : 0};
    result.score = calculateScore(result.rank, result.kickers);
    return result;
}

// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש
// »²§U¨ח¼ֶ¹ך²{
// שששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששששש

int HandEvaluator::calculateScore(HandRank rank, const std::array<int, 5>& kickers) {
    int score = static_cast<int>(rank) * 1000000; // µP«¬°עֲ¦₪ְ¼ֶ
    
    // ¥[₪W½נ¸}µP₪ְ¼ֶ
    score += kickers[0] * 10000;
    score += kickers[1] * 1000;
    score += kickers[2] * 100;
    score += kickers[3] * 10;
    score += kickers[4];
    
    return score;
}

std::string HandEvaluator::handRankToString(HandRank rank) {
    switch (rank) {
        case HandRank::HIGH_CARD: return "High Card";
        case HandRank::ONE_PAIR: return "One Pair";
        case HandRank::TWO_PAIR: return "Two Pair";
        case HandRank::THREE_OF_KIND: return "Three of a Kind";
        case HandRank::STRAIGHT: return "Straight";
        case HandRank::FLUSH: return "Flush";
        case HandRank::FULL_HOUSE: return "Full House";
        case HandRank::FOUR_OF_KIND: return "Four of a Kind";
        case HandRank::STRAIGHT_FLUSH: return "Straight Flush";
        case HandRank::ROYAL_FLUSH: return "Royal Flush";
        default: return "Unknown";
    }
}