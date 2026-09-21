#pragma once
#include <JuceHeader.h>

/**
 * CorrelationMeter
 * Displays a single-band correlation value as a bi-directional segmented
 * LED-style meter.
 *
 * Visual spec (from Requirements):
 *   Size  : 300 x 30 px
 *   Blocks: 30 segments of 8px width, 1px side margin each
 *   Range : -1 (left) to +1 (right), zero at center (block 15)
 *   Color : supplied via constructor (Low=Red, Mid=Green, High=Blue)
 *   Alpha : 70% for lit blocks; ~15% for dim blocks
 *   Glow  : Gaussian-blur-like soft outer glow on lit blocks
 *   Labels: -1 (left), 0 (center), +1 (right) drawn below meter
 */
class CorrelationMeter : public juce::Component
{
public:
    static constexpr int numBlocks   = 30;
    static constexpr int blockWidth  = 8;
    static constexpr int blockMargin = 1;   // each side

    explicit CorrelationMeter(juce::Colour colour)
        : bandColor(colour.withAlpha(0.70f)),
          currentCorrelation(0.0f)
    {
        meterSize = juce::Rectangle<int>(0, 0, 300, 30);
    }

    ~CorrelationMeter() override = default;

    //=========================================================================
    void updateValue(float newValue)
    {
        currentCorrelation = juce::jlimit(-1.0f, 1.0f, newValue);
        repaint();
    }

    //=========================================================================
    void paint(juce::Graphics& g) override
    {
        const auto bounds   = getLocalBounds();
        const int  meterH   = bounds.getHeight() - 16; // leave room for labels
        const int  meterY   = 0;

        const float blockStep = static_cast<float>(bounds.getWidth()) / numBlocks;
        const int   centerIdx = numBlocks / 2; // block index 15 = zero crossing

        // Map correlation [-1,+1] to block count filled from center
        // Positive: fill right of center; Negative: fill left of center
        const float normalized = currentCorrelation; // already -1..+1
        const int   litHalfCount = static_cast<int>(std::abs(normalized) * centerIdx + 0.5f);
        const bool  isPositive   = normalized >= 0.0f;

        for (int i = 0; i < numBlocks; ++i)
        {
            // Is this block lit?
            bool lit = false;
            if (isPositive)
                lit = (i >= centerIdx) && (i < centerIdx + litHalfCount);
            else
                lit = (i < centerIdx)  && (i >= centerIdx - litHalfCount);

            const float bx = blockStep * i + blockMargin;
            const float bw = blockStep - blockMargin * 2.0f;

            juce::Rectangle<float> blockRect(bx, static_cast<float>(meterY),
                                              bw, static_cast<float>(meterH));

            if (lit)
            {
                // Draw glow: soft larger rectangle behind
                const juce::Colour glowColor = bandColor.withAlpha(0.25f);
                g.setColour(glowColor);
                g.fillRoundedRectangle(blockRect.expanded(2.0f, 2.0f), 2.0f);

                // Draw lit block
                g.setColour(bandColor);
                g.fillRoundedRectangle(blockRect, 1.5f);

                // Top specular highlight
                g.setColour(juce::Colours::white.withAlpha(0.15f));
                g.fillRoundedRectangle(blockRect.removeFromTop(blockRect.getHeight() * 0.3f), 1.0f);
            }
            else
            {
                // Dim (unlit) block
                g.setColour(bandColor.withAlpha(0.12f));
                g.fillRoundedRectangle(blockRect, 1.5f);
            }
        }

        // Center tick mark
        const float cx = blockStep * centerIdx;
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillRect(juce::Rectangle<float>(cx, 0.0f, 1.0f, static_cast<float>(meterH)));

        // Labels: -1, 0, +1
        drawLabels(g);
    }

    void drawLabels(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds();
        const int  labelY = bounds.getHeight() - 14;

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::Font(10.0f));

        g.drawText("-1",  bounds.getX(),                           labelY, 20, 14, juce::Justification::centred);
        g.drawText("0",   bounds.getCentreX() - 8,                 labelY, 16, 14, juce::Justification::centred);
        g.drawText("+1",  bounds.getRight() - 20,                  labelY, 20, 14, juce::Justification::centred);
    }

    void resized() override
    {
        meterSize = getLocalBounds();
    }

private:
    juce::Colour           bandColor;
    float                  currentCorrelation;
    juce::Rectangle<int>   meterSize;
    std::vector<juce::Rectangle<float>> blocks; // cached geometry (optional optimization)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CorrelationMeter)
};
