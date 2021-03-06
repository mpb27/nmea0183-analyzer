#ifndef NMEA_ANALYZER_RESULTS
#define NMEA_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define FRAMING_ERROR_FLAG ( 1 << 0 )
#define PARITY_ERROR_FLAG ( 1 << 1 )
//#define MP_MODE_ADDRESS_FLAG ( 1 << 2 )

class NmeaAnalyzer;
class NmeaAnalyzerSettings;

class NmeaAnalyzerResults : public AnalyzerResults
{
public:
	NmeaAnalyzerResults( NmeaAnalyzer* analyzer, NmeaAnalyzerSettings* settings );
	virtual ~NmeaAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	
protected:  //vars
	NmeaAnalyzerSettings* mSettings;
	NmeaAnalyzer* mAnalyzer;
};


enum class ResultType { SerialByte = 0, NmeaData = 1 };

struct SerialByte
{
	enum class SerialError { None, Framing, Parity };

	unsigned char data = 0;
	SerialError   error = SerialError::None;
	U64			  start_sample = 0;
	U64           end_sample = 0;
};

struct NmeaData
{
	U64			start_sample;
	U64			end_sample;
	std::string nmea;
};


#endif //NMEA_ANALYZER_RESULTS
