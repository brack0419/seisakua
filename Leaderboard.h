#pragma once
#include <string>
#include <vector>
#include <functional>

struct RankingData {
    std::string name;
    float time;
    int kills;
};

class Leaderboard {
public:
    // サーバーのURL (https://script.google.com/macros/s/AKfycbwuCi3A2bVHpLrbTOMAT0Lzmlx9nAH5SMdir-9lqmFTCtjxKjL2EMJjaadJKqJ0kIjL4A/exec の形式)
    // ★ここにステップ1で取得したURLのドメイン以降("/macros/..."から)をセットします
    static const std::wstring SERVER_PATH;
    static const std::wstring SERVER_HOST; // "script.google.com"

    // スコア送信 (非同期で行うためスレッドを使用推奨)
    static void SendScore(const std::string& name, float time, int kills);

    // ランキング取得
    static void FetchRanking(std::function<void(const std::vector<RankingData>&)> onComplete);
};