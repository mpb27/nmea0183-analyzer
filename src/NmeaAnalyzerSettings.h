#ifndef SERIAL_ANALYZER_SETTINGS
#define SERIAL_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class NmeaAnalyzerSettings : public AnalyzerSettings
{
public:
	NmeaAnalyzerSettings();
	virtual ~NmeaAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	Channel mInputChannel;
	U32 mBitRate;
	U32 mBitsPerTransfer;
	double mStopBits;
	AnalyzerEnums::Parity mParity;
	bool mUseAutobaud;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList > mBitsPerTransferInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mStopBitsInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mParityInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		mUseAutobaudInterface;
};

#endif //SERIAL_ANALYZER_SETTINGS
