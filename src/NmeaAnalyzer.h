#ifndef SERIAL_ANALYZER_H
#define SERIAL_ANALYZER_H

#include <Analyzer.h>
#include "NmeaAnalyzerResults.h"
#include "NmeaSimulationDataGenerator.h"

class NmeaAnalyzerSettings;
class NmeaAnalyzer : public Analyzer2
{
public:
	NmeaAnalyzer();
	virtual ~NmeaAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
	

	 
#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'NmeaAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected: //functions
	U32 CalculateBitOffset();
	SerialByte NmeaAnalyzer::SampleByte();

protected: //vars
	std::auto_ptr< NmeaAnalyzerSettings > mSettings;
	std::auto_ptr< NmeaAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	NmeaSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	U32 mBitOffset;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SERIAL_ANALYZER_H
