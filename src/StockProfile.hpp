// StockProfile.hpp
#pragma once

#include <string>
#include <vector>
#include <random>
#include <functional>
#include "OrderGenerator.hpp"

// ── Market regime — defines how the price moves right now ────────────────────
enum class Regime {
    Momentum,       // price trends — moves reinforce themselves
    MeanReversion,  // price oscillates around a stable midpoint
    Volatile,       // large random moves, unpredictable direction
    Quiet           // low volatility, tight spread, low activity
};

// ── A single risk event that can fire during simulation ──────────────────────
struct RiskEvent {
    std::string name;           // display name — shown in news banner
    std::string headline;       // e.g. "NOVA: earnings beat — gap up incoming"
    double      priceDelta;     // immediate price shock — positive or negative
    double      volMultiplier;  // how much volatility spikes (1.0 = no change)
    double      spreadMultiplier; // how much spread widens
    int         durationOrders; // how many orders the effect lasts
    bool        affectsAll;     // true = all stocks affected, false = this stock only
};

// ── Regime transition probabilities ─────────────────────────────────────────
struct RegimeWeights {
    double toMomentum;
    double toMeanReversion;
    double toVolatile;
    double toQuiet;
};

// ── Full personality definition for one stock ─────────────────────────────────
struct StockProfile {
    // Identity
    std::string ticker;
    std::string name;
    std::string description;
    std::string sector;

    // Base market parameters
    double startPrice;
    double baseVolatility;
    double baseSpread;
    double baseDrift;           // long-run directional bias (0 = neutral)
    double meanReversionTarget; // price MeanReversion snaps toward
    double meanReversionSpeed;  // how aggressively it snaps (0-1)

    // Regime configuration
    Regime              startRegime;
    RegimeWeights       regimeWeights;   // transition probabilities from current
    int                 regimeCheckEvery;// check for regime change every N orders

    // Risk events specific to this stock
    std::vector<RiskEvent> events;
    double                 eventProbability; // per regimeCheckEvery orders

    // Momentum parameters
    double momentumDecay;       // how fast momentum fades (0-1, lower = longer trend)
    double momentumStrength;    // how much prior move influences next move

    // Display color hint for GUI (ImGui color packed as float[4])
    float color[4];

    // ── Derive a GeneratorConfig for this stock at current state ─────────────
    GeneratorConfig toGeneratorConfig(double currentPrice,
                                      Regime  currentRegime,
                                      double  volMultiplier  = 1.0,
                                      double  spreadMultiplier = 1.0) const
    {
        GeneratorConfig cfg;
        cfg.midPrice  = currentPrice;
        cfg.tickSize  = 0.01;

        double regimeVolMod  = 1.0;
        double regimeSpreadMod = 1.0;
        double regimeDriftMod  = 1.0;

        switch (currentRegime) {
            case Regime::Momentum:
                regimeVolMod   = 1.2;
                regimeSpreadMod = 1.1;
                regimeDriftMod  = 2.5;   // stronger directional push
                break;
            case Regime::MeanReversion:
                regimeVolMod   = 0.7;
                regimeSpreadMod = 0.9;
                regimeDriftMod  = 0.3;   // drift suppressed, spring dominates
                break;
            case Regime::Volatile:
                regimeVolMod   = 3.0;
                regimeSpreadMod = 2.5;
                regimeDriftMod  = 1.0;
                break;
            case Regime::Quiet:
                regimeVolMod   = 0.3;
                regimeSpreadMod = 0.6;
                regimeDriftMod  = 0.2;
                break;
        }

        cfg.priceVolatility = baseVolatility
                            * regimeVolMod
                            * volMultiplier;

        cfg.spread = baseSpread
                   * regimeSpreadMod
                   * spreadMultiplier;

        cfg.marketOrderPct = (currentRegime == Regime::Volatile) ? 0.35 : 0.20;
        cfg.cancelPct      = (currentRegime == Regime::Volatile) ? 0.20 : 0.10;
        cfg.minQty         = 1;
        cfg.maxQty         = 100;

        return cfg;
    }
};

// ── Stock profile factory — returns all 6 pre-built personalities ─────────────
inline std::vector<StockProfile> buildStockUniverse() {
    std::vector<StockProfile> stocks;

    // ── NOVA — high volatility tech ───────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "NOVA";
        s.name        = "Nova Systems Inc.";
        s.description = "High-growth tech. Wide spread, strong momentum, "
                        "violent earnings reactions. Rewarding if you ride "
                        "the trend. Punishing if you fight it.";
        s.sector      = "Technology";

        s.startPrice           = 420.00;
        s.baseVolatility       = 0.12;
        s.baseSpread           = 0.20;
        s.baseDrift            = 0.002;
        s.meanReversionTarget  = 420.00;
        s.meanReversionSpeed   = 0.01;   // barely reverts — momentum dominates

        s.startRegime          = Regime::Momentum;
        s.regimeWeights        = { 0.50, 0.10, 0.25, 0.15 };
        s.regimeCheckEvery     = 2000;
        s.momentumDecay        = 0.92;
        s.momentumStrength     = 0.40;
        s.eventProbability     = 0.15;

        s.events = {
            { "Earnings beat",   "NOVA: earnings beat — gap up incoming",
               +8.00, 2.5, 2.0, 3000, false },
            { "Earnings miss",   "NOVA: earnings miss — selling pressure",
              -12.00, 3.0, 2.5, 4000, false },
            { "Analyst upgrade", "NOVA: analyst upgrades to strong buy",
               +4.00, 1.5, 1.3, 1500, false },
            { "Insider selling", "NOVA: large insider sale detected",
               -5.00, 2.0, 1.8, 2000, false },
        };

        s.color[0]=0.49f; s.color[1]=0.40f; s.color[2]=0.87f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    // ── UTIL — slow utility ───────────────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "UTIL";
        s.name        = "United Utilities Corp.";
        s.description = "Slow moving utility stock. Extremely tight spread, "
                        "strong mean reversion. Safe during crashes. "
                        "Boring but consistent — the professional's choice.";
        s.sector      = "Utilities";

        s.startPrice           = 88.00;
        s.baseVolatility       = 0.012;
        s.baseSpread           = 0.02;
        s.baseDrift            = 0.0;
        s.meanReversionTarget  = 88.00;
        s.meanReversionSpeed   = 0.15;   // strong spring

        s.startRegime          = Regime::MeanReversion;
        s.regimeWeights        = { 0.05, 0.70, 0.10, 0.15 };
        s.regimeCheckEvery     = 5000;
        s.momentumDecay        = 0.60;
        s.momentumStrength     = 0.05;
        s.eventProbability     = 0.04;

        s.events = {
            { "Rate cut",   "FED: rate cut announced — utilities rally",
               +2.50, 1.2, 1.0, 2000, true  },
            { "Rate hike",  "FED: rate hike announced — utilities pressured",
               -2.00, 1.3, 1.1, 2000, true  },
            { "Dividend",   "UTIL: special dividend declared",
               +1.00, 0.8, 0.9, 1000, false },
        };

        s.color[0]=0.11f; s.color[1]=0.62f; s.color[2]=0.46f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    // ── MEME — chaotic retail ─────────────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "MEME";
        s.name        = "Memelon Corp.";
        s.description = "Retail-driven chaos. Normal most of the time, then "
                        "a vol spike triples volatility with no warning. "
                        "High risk, high reward. Not for the faint-hearted.";
        s.sector      = "Consumer";

        s.startPrice           = 34.00;
        s.baseVolatility       = 0.06;
        s.baseSpread           = 0.15;
        s.baseDrift            = 0.0;
        s.meanReversionTarget  = 34.00;
        s.meanReversionSpeed   = 0.03;

        s.startRegime          = Regime::Quiet;
        s.regimeWeights        = { 0.20, 0.10, 0.50, 0.20 };
        s.regimeCheckEvery     = 800;    // checks frequently — regime flips fast
        s.momentumDecay        = 0.75;
        s.momentumStrength     = 0.25;
        s.eventProbability     = 0.25;   // events fire often

        s.events = {
            { "Short squeeze",   "MEME: short squeeze initiated — vol spike",
               +6.00, 4.0, 3.5, 2000, false },
            { "Influencer pump", "MEME: viral social post — retail surge",
               +3.00, 3.0, 2.5, 1500, false },
            { "Margin calls",    "MEME: forced liquidations — price collapse",
               -8.00, 4.5, 4.0, 2500, false },
            { "Halt lifted",     "MEME: trading halt lifted — chaos resumes",
                0.00, 5.0, 5.0,  800, false },
        };

        s.color[0]=0.85f; s.color[1]=0.35f; s.color[2]=0.19f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    // ── BANK — financial sector ───────────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "BANK";
        s.name        = "First National Bancorp";
        s.description = "Financial sector proxy. Medium volatility with "
                        "strong rate sensitivity. Anti-correlates with CRUDE "
                        "during rate shocks. Teaches hedging naturally.";
        s.sector      = "Financials";

        s.startPrice           = 156.00;
        s.baseVolatility       = 0.04;
        s.baseSpread           = 0.05;
        s.baseDrift            = 0.001;
        s.meanReversionTarget  = 156.00;
        s.meanReversionSpeed   = 0.05;

        s.startRegime          = Regime::Quiet;
        s.regimeWeights        = { 0.20, 0.35, 0.20, 0.25 };
        s.regimeCheckEvery     = 3000;
        s.momentumDecay        = 0.80;
        s.momentumStrength     = 0.15;
        s.eventProbability     = 0.08;

        s.events = {
            { "Rate shock up",  "FED: surprise rate hike — banks rally",
               +5.00, 1.8, 1.5, 3000, true  },
            { "Rate shock down", "FED: emergency cut — bank margins compressed",
               -4.00, 2.0, 1.6, 3000, true  },
            { "Credit event",   "BANK: credit loss disclosed — selloff",
               -9.00, 3.0, 2.5, 4000, false },
            { "Buyback",        "BANK: $2B share buyback announced",
               +3.00, 1.2, 1.0, 2000, false },
        };

        s.color[0]=0.22f; s.color[1]=0.47f; s.color[2]=0.87f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    // ── CRUDE — commodity proxy ───────────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "CRUDE";
        s.name        = "Crude Energy ETF";
        s.description = "Commodity-linked ETF. Supply shock events cause "
                        "instant price gaps. Anti-correlates with BANK "
                        "on rate events. Teaches commodity dynamics.";
        s.sector      = "Energy";

        s.startPrice           = 78.00;
        s.baseVolatility       = 0.07;
        s.baseSpread           = 0.10;
        s.baseDrift            = 0.0;
        s.meanReversionTarget  = 78.00;
        s.meanReversionSpeed   = 0.04;

        s.startRegime          = Regime::Momentum;
        s.regimeWeights        = { 0.30, 0.20, 0.30, 0.20 };
        s.regimeCheckEvery     = 2500;
        s.momentumDecay        = 0.85;
        s.momentumStrength     = 0.30;
        s.eventProbability     = 0.10;

        s.events = {
            { "Supply cut",    "OPEC: surprise supply cut — crude spikes",
               +7.00, 2.5, 2.0, 3500, false },
            { "Supply glut",   "OPEC: production increase — crude drops",
               -6.00, 2.0, 1.8, 3000, false },
            { "Geopolitical",  "GEOPOLITICAL: pipeline disruption risk",
               +4.00, 3.0, 2.5, 2000, false },
            { "Rate shock",    "FED: rate hike — dollar up, crude down",
               -3.00, 1.5, 1.3, 2000, true  },
        };

        s.color[0]=0.73f; s.color[1]=0.46f; s.color[2]=0.08f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    // ── DEFN — defense / safe haven ───────────────────────────────────────────
    {
        StockProfile s;
        s.ticker      = "DEFN";
        s.name        = "Aegis Defense Systems";
        s.description = "Defense contractor. Lowest volatility. Slight "
                        "negative correlation with market crashes — rises "
                        "when others fall. The safe haven stock.";
        s.sector      = "Defense";

        s.startPrice           = 210.00;
        s.baseVolatility       = 0.015;
        s.baseSpread           = 0.04;
        s.baseDrift            = 0.0005;
        s.meanReversionTarget  = 210.00;
        s.meanReversionSpeed   = 0.08;

        s.startRegime          = Regime::Quiet;
        s.regimeWeights        = { 0.05, 0.25, 0.05, 0.65 };
        s.regimeCheckEvery     = 6000;
        s.momentumDecay        = 0.50;
        s.momentumStrength     = 0.05;
        s.eventProbability     = 0.05;

        s.events = {
            { "Defense contract", "DEFN: $4B government contract awarded",
               +8.00, 1.3, 1.1, 2000, false },
            { "Budget cut",       "CONGRESS: defense budget reduced",
               -5.00, 1.5, 1.3, 2000, false },
            { "Market crash",     "MARKET: risk-off — safe havens bid",
               +3.00, 0.8, 0.8, 3000, true  },
        };

        s.color[0]=0.53f; s.color[1]=0.53f; s.color[2]=0.53f; s.color[3]=1.0f;
        stocks.push_back(s);
    }

    return stocks;
}