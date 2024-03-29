 /*
 *    QRAP Project
 *
 *    Version     : 0.1
 *    Date        : 2008/04/01
 *    License     : GNU GPLv3
 *    File        : cPlotTask.cpp
 *    Copyright   : (c) University of Pretoria
 *    Author      : Magdaleen Ballot (magdaleen.ballot@up.ac.za)
 *    Description : The entry class for all propagation prediction plots
 *
 **************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; See the GNU General Public License for      *
 *   more details                                                          *
 *                                                                         *
 ************************************************************************* */

#include "cPlotTask.h"
#include <stdio.h>
//#include <algorithm>

using namespace Qrap;
using namespace std;

//************************************************************************
cPlotTask::cPlotTask()
{
	mPlotType = Cov;
	mUnits = dBm;
	mInstCounter =0;
	mRqSN = 8;			//dB
	mRxMin = -110;			//dBm
	mFadeMargin = 3;		//dB
	mRqCIco = 9;			//dB
	mRqCIad = -9;			//dB
	mRqEbNo = 8;			//dB
	mNoiseLevel = 6;		//dBm
	mkFactorServ = 1;
	mkFactorInt = 2.5;
	mDEMsource = 1;
	mDownlink = true;
	mClutterSource = 2;
	mUseClutter = false;
	mPlotResolution = 90;
	mMinAngleRes = 1;
	mNumAngles = 360;
	mMobile.sInstKey = 0;
	mMobile.sEIRP = 33;
	mMobile.sTxPower = 2;
	mMobile.sTxSysLoss = 0;
	mMobile.sRxSysLoss = 0;
	mMobile.sRxSens = -104;
	mMobile.sPatternKey = 0;
	mMobile.sMobileHeight = 1;
	mPlot = new_Float2DArray(2,2);
	mSupportPlot = new_Float2DArray(2,2);
	mDir = "/home/justin/Data/qrap/Predictions/";
	mOutputFile = "Output";	
}

//**************************************************************************
cPlotTask::~cPlotTask()
{
	delete_Float2DArray(mPlot);
	delete_Float2DArray(mSupportPlot);
	mFixedInsts.clear();
	for (unsigned i=0; i<mActiveRasters.size(); i++)
		delete_Float2DArray(mActiveRasters[i].sRaster);
	mActiveRasters.clear();
}

//**************************************************************************
//  how to use
//Prediction.SetPlotTask(	mPlotType, DisplayUnits, DownLink,
//            RequiredSignalToNoise, RequiredMinimumReceiverLevel,
//            FadeMargin, RequiredCoChannelCarrierToInterference,
//            RequiredAdjacentCarrierToInterference, RequiredEnergyPerBitToNoiseRatio,
//            NoiseLevel, kFactorForServers, kFactorForInterferers,
//            DEMsourceList, ClutterSourceList, UseClutterDataInPathLossCalculations,
//            NorthWestCorner, SouthEastCorner, PlotResolution,
//            MinimumAngularResolution, MobileInstallationKey,
//            NumberOfFixedInstallations, FixedInstallationKeys,
//            CoverangeRanges, // In Kilometer
//            DirectoryToStoreResult,
//            OutputFileForResult);

bool cPlotTask::SetPlotTask(	ePlotType PlotType,
				eOutputUnits DisplayUnits,
				bool Downlink,
				double RequiredSignalToNoise,
				double RequiredMinimumReceiverLevel,
				double FadeMargin,
				double RequiredCoChannelCarrierToInterference,
				double RequiredAdjacentCarrierToInterference,
				double RequiredEnergyPerBitToNoiseRatio,
				double NoiseLevel,
				double kFactorForServers,
				double kFactorForInterferers,
				short int DEMsourceList,
				short int ClutterSourceList,
				bool UseClutterDataInPathLossCalculations,
				cGeoP NorthWestCorner,
				cGeoP SouthEastCorner,
				double PlotResolution,
				double MinimumAngularResolution,
				unsigned MobileInstallationKey,
				unsigned NumberOfFixedInstallations,
				unsigned *FixedInstallationKeys,
				double *CoverangeRanges, // In Kilometer
				string DirectoryToStoreResult,
				string OutputFileForResult)
{
	string err = "Preparing for Requested Prediction.";
//	QRAP_INFO(err.c_str());
	cout << err << endl;
	mPlotType 	= PlotType;							
	mUnits 		= DisplayUnits;							
	mDownlink 	= Downlink;							
	mRqSN 		= RequiredSignalToNoise;							
	mRxMin 		= RequiredMinimumReceiverLevel;							
	mFadeMargin	= FadeMargin;							
	mRqCIco 	= RequiredCoChannelCarrierToInterference;							
	mRqCIad		= RequiredAdjacentCarrierToInterference;							
	mRqEbNo 	= RequiredEnergyPerBitToNoiseRatio;							
	mNoiseLevel	= NoiseLevel;		
	mkFactorServ	= kFactorForServers;						
	mkFactorInt	= kFactorForInterferers;							
	mDEMsource	= DEMsourceList;				
	mClutterSource  = ClutterSourceList;					
	mUseClutter	= UseClutterDataInPathLossCalculations;							
	mNorthWest	= NorthWestCorner;				
	mSouthEast	= SouthEastCorner;
	double N, W, S, E;
	cout << "North West corner: " << endl;
	NorthWestCorner.Display();
	cout << "South East corner: " << endl;
	SouthEastCorner.Display();
	mNorthWest.Get(N,W);
	mSouthEast.Get(S,E);
	mSouthWest.Set(S,W);
	mNorthEast.Set(N,E); //! 4个角
    mPlotResolution=PlotResolution;	//! DEM mapfile resolution using now from US is 90m
	mMinAngleRes=MinimumAngularResolution;			
	mNumAngles = (unsigned)(360.0/mMinAngleRes);
	mMobile.sInstKey=MobileInstallationKey;				
	mNumFixed=NumberOfFixedInstallations;
	
	mMaxRange=0;
	mFixedInsts.clear();
	for (unsigned i=0; i<mNumFixed; i++)
	{
		tFixed tempInst;
		tempInst.sInstKey = FixedInstallationKeys[i];
		tempInst.sRange = CoverangeRanges[i];
        tempInst.sRange *= 1000; //! transform to meter
		tempInst.sCentroidX = 0;
		tempInst.sCentroidY = 0;
		tempInst.sPixelCount = 0;
		if (tempInst.sRange>mMaxRange) 
			mMaxRange = tempInst.sRange;
        mFixedInsts.push_back(tempInst); //! push in to all sector installtions
	}
	if (mMaxRange>0)
	{
		if (mMinAngleRes<(270*mPlotResolution/mMaxRange/PI))
		{
            if (mPlotResolution<0.5) mPlotResolution=20;  //! unit is meter
			int mNumAngles = (int)ceil(1.5*PI*mMaxRange/mPlotResolution);
			mMinAngleRes=360.0/(double)mNumAngles;
		}
	}
	if (mMinAngleRes<0.0001)
	{
		mMinAngleRes=1;
		mNumAngles = 360;
	}
	mNumAngles = (unsigned)(360.0/mMinAngleRes);
	if (mNumAngles<4)
	{
		mMinAngleRes=20;
		mNumAngles = 18;		
	}
	
	mDir=DirectoryToStoreResult;			
	mOutputFile=OutputFileForResult;	
//	double WE = max(mNorthWest.Distance(mNorthEast), 
//					mSouthWest.Distance(mSouthEast));
//	mRows = (int)ceil(mNorthWest.Distance(mSouthWest)/mPlotResolution);
//	mCols = (int)ceil(WE/mPlotResolution);
    mNSres = mPlotResolution*cDegResT; //! cDegResT=0.000833/90.0; 90米代表的经纬度 // derive resolution in lat or long
	mEWres = mPlotResolution*cDegResT;
	mRows = (unsigned)((N-S)/(mNSres)); //! 行： 纬度差/纬度分辨率 ----关联栅格
	mCols = (unsigned)((E-W)/(mEWres)); //! 列
	delete_Float2DArray(mPlot);
	delete_Float2DArray(mSupportPlot);
    mPlot = new_Float2DArray(mRows,mCols); //! 按经纬度分辨率regenerate的矩阵
	mSupportPlot = new_Float2DArray(mRows,mCols); //!
	cGeoP MidNorth(N,(W+E)/2, DEG);
	cGeoP Mid((N+S)/2,(E+W)/2,DEG);
	mCentMer = Mid.DefaultCentMer(WGS84GC);
//	mNorthWest.FromHere(MidNorth,mCols/2.0*mPlotResolution,270.0);
	double dummy;
	if (mCols<mRows)
		mNorthWest.Get(mCurrentEdge, dummy); //!西北 get(纬度，经度)
	else 
		mNorthWest.Get(dummy, mCurrentEdge);
	bool FoundRasterSet = mDEM.SetRasterFileRules(mDEMsource); //! DEM file id
	if (!FoundRasterSet)
	{
		string err = "Trouble getting DEM list. Using default";
		QRAP_WARN(err.c_str());
	}
	if (mUseClutter) //! skip
	{
		FoundRasterSet = mClutter.SetRasterFileRules(mClutterSource);
		mUseClutter = (FoundRasterSet)&&(UseClutterDataInPathLossCalculations);
		if (!FoundRasterSet)
		{
			string err ="  Trouble getting Clutter list. Not using clutter.";
			QRAP_WARN(err.c_str());
		}
	}
	if (mUseClutter)	//! skip
		mClutterClassGroup = mClutter.GetClutterClassGroup();
	mUseClutter = (mUseClutter) && (mClutterClassGroup>0);
	

	cout << "North West corner: " << endl;
	NorthWestCorner.Display();
	cout << "South East corner: " << endl;
	SouthEastCorner.Display();


	// \TODO: Remove at some stage
	//DEBUGGING !!!!!!!!!!!!!
	// This does not write Clutter related stuff
	bool WriteFile = true;
	if (WriteFile)
	{
		string File = mDir;
		File += "/ExamplePlotRequest";
		FILE *fp = fopen(File.c_str(),"w");
		if (fp!=NULL)
		{
			string temp;
			switch (mPlotType)
			{
				case DEM:		temp = "DEM"; 		break;
				case Cov:		temp = "Coverage"; 	break; //!
				case PrimServer:	temp = "PrimServer"; 	break;
				case SecondServer:	temp = "SecondServer"; 	break;
				case NumServers:	temp = "NumServers"; 	break;
				case IntRatioCo:	temp = "IntRatioCo";	break;
				case IntRatioAd:	temp = "IntRatioAd"; 	break;
				case IntAreas:		temp = "IntAreas"; 	break;
				case NumInt:		temp = "NumInt";	break;
				case PrimIntCo:		temp = "PrimIntCo";	break;
				case PrimIntAd:		temp = "PrimOntAd";	break;
				case SN:		temp = "SN";		break;
				case EbNo:		temp = "EbNo";		break;
				case CellCentroid:	temp = "CellCentroid";	break;
				case TrafficDist:	temp = "TrafficDist";	break;
				case ServiceLimits:	temp = "ServiceLimits";	break;
			}
			fprintf(fp,"%s\n",temp.c_str());
			fprintf(fp,"dBm\n");
			fprintf(fp,"Downlink\n");
			fprintf(fp,"SN %f\n",RequiredSignalToNoise);
			fprintf(fp,"RxMin %f\n",RequiredMinimumReceiverLevel);
			fprintf(fp,"FadeMargin %f\n", FadeMargin);
			fprintf(fp,"CIco %f\n", RequiredCoChannelCarrierToInterference);
			fprintf(fp,"CIad %f\n", RequiredAdjacentCarrierToInterference);
			fprintf(fp,"EbNo %f\n", RequiredEnergyPerBitToNoiseRatio);
			fprintf(fp,"Noise %f\n", NoiseLevel);
			fprintf(fp,"kServ %f\n",  kFactorForServers);
			fprintf(fp,"kInt %f\n", kFactorForInterferers);
			fprintf(fp,"Resolution %f\n", PlotResolution);
			fprintf(fp,"MinAngleRes %f\n", MinimumAngularResolution);
			fprintf(fp,"Mobile %d\n", MobileInstallationKey);
			fprintf(fp,"NumFixed %d\n", NumberOfFixedInstallations);
			fprintf(fp,"Fixed\n");
			for (unsigned i = 0; i < NumberOfFixedInstallations ; i++)
			{
				fprintf(fp,"%d %f\n",FixedInstallationKeys[i],CoverangeRanges[i]);
			}
			fprintf(fp,"Area\n");
			fprintf(fp,"N %f S %f W %f E %f\n",N,S,W,E);
			fprintf(fp,"DEM %d\n", mDEMsource);
			if (mUseClutter)
			{
				fprintf(fp,"UseClutter TRUE\n");
				fprintf(fp,"Clutter %d\n", mClutterSource);
			}
			else fprintf(fp,"UseClutter FALSE\n");
			fprintf(fp,"OutputDir %s\n",mDir.c_str());
			fprintf(fp,"OutputFile %s\n",mOutputFile.c_str());
			
			fclose(fp);
		}
	}
	
	bool ReturnValue = GetDBinfo(); //! 获取计算所需信息 重要
	return ReturnValue;
}// end cPlotTask::SetPlotTask


//**************************************************************************
bool cPlotTask::ReadPlotRequest(const char *filename)
{
	
	char *temp;
	temp = new char[NAME_MAX+1];
	unsigned i;
	bool FoundRasterSet = TRUE;
	
	for (i=0;i<=NAME_MAX;i++)
		temp[i] ='\0';
	
	ifstream infile(filename);
	if (!infile)
	{
		string err = "Error opening file: ";
		err+=filename;
		QRAP_ERROR(err.c_str());
		return false;
	}
		
	infile >> temp;
	if (infile.eof())
	{
		string err = "Error reading file: ";
		err+=filename;
		err+=" before PlotType. en of file reached";
		QRAP_ERROR(err.c_str());
		return false;
	}
	else if (!strcasecmp(temp,"DEM"))
		mPlotType = DEM;
	else if (!strcasecmp(temp,"Coverage"))
		mPlotType = Cov;
	else if (!strcasecmp(temp,"PrimServer"))
		mPlotType = PrimServer;
	else if (!strcasecmp(temp,"CellCentroid"))
		mPlotType = CellCentroid;
	else if (!strcasecmp(temp,"TrafficDist"))
		mPlotType = TrafficDist;
	else if (!strcasecmp(temp,"SecondServer"))
		mPlotType = SecondServer;	
	else if (!strcasecmp(temp,"NumServers"))
		mPlotType = NumServers;	
	else if (!strcasecmp(temp,"IntRatioCo"))
		mPlotType = IntRatioCo;
	else if (!strcasecmp(temp,"IntRatioAd"))
		mPlotType = IntRatioAd;
	else if (!strcasecmp(temp,"IntAreas"))
		mPlotType = IntAreas;
	else if (!strcasecmp(temp,"NumInt"))
		mPlotType = NumInt;
	else if (!strcasecmp(temp,"PrimIntCo"))
		mPlotType = PrimIntCo;
	else if (!strcasecmp(temp,"PrimIntAd"))
		mPlotType = PrimIntAd;
	else if (!strcasecmp(temp,"SN"))
		mPlotType = SN;
	else if (!strcasecmp(temp,"EbNo"))
		mPlotType = EbNo;
	else if (!strcasecmp(temp,"ServiceLimits"))
		mPlotType = ServiceLimits;
	else
	{
		string err = "Error reading file: ";
		err+=filename;
		err+=" at PlotType. Using default";
		QRAP_WARN(err.c_str());
	}
	
	infile >> temp;
	if (infile.eof())
	{
		string err = "Error reading file: ";
		err+=+filename;
		err+=" at Units. End of file reached";
		QRAP_WARN(err.c_str());
		return false;
	}
	else if (!strcasecmp(temp,"dB"))
		mUnits = dB;
	else if (!strcasecmp(temp,"dBW"))
		mUnits = dBW;
	else if (!strcasecmp(temp,"dBm"))
		mUnits = dBm;	
	else if (!strcasecmp(temp,"dBuV"))
		mUnits = dBuV;
	else if (!strcasecmp(temp,"dBuVm"))
		mUnits = dBuVm;
	else if (!strcasecmp(temp,"dBWm2Hz"))
		mUnits = dBWm2Hz;
	else if (!strcasecmp(temp,"dBWm2"))
		mUnits = dBWm2;
	else
	{
		string err = "Error reading file: ";
		err+=filename;
		err+=" at Units. Using default";
		QRAP_WARN(err.c_str());
	}
	infile >> temp;
	if (!strcasecmp(temp, "Downlink"))
	{
		mDownlink = true;
		infile >> temp;
	}
	else if (!strcasecmp(temp, "Uplink"))
	{
		mDownlink = false;
		infile >> temp;
	}
	if (!strcasecmp(temp, "SN"))
	{
		infile >> mRqSN;
		infile >> temp;
	}
	if (!strcasecmp(temp, "RxMin"))
	{
		infile >> mRxMin;
		infile >> temp;
	}
	if (!strcasecmp(temp, "FadeMargin"))
	{
		infile >> mFadeMargin;
		infile >> temp;
	}
	if (!strcasecmp(temp, "CIco"))
	{
		infile >> mRqCIco;
		infile >> temp;
	}
	if (!strcasecmp(temp, "CIad"))
	{
		infile >> mRqCIad;
		infile >> temp;
	}
	if (!strcasecmp(temp, "EbNo"))
	{
		infile >> mRqEbNo;
		infile >> temp;
	}
	if (!strcasecmp(temp, "Noise"))
	{
		infile >> mNoiseLevel;
		infile >> temp;
	}
	if (!strcasecmp(temp, "kServ"))
	{
		infile >> mkFactorServ;
		infile >> temp;
	}
	if (!strcasecmp(temp, "kInt"))
	{
		infile >> mkFactorInt;
		infile >> temp;
	}
	if (!strcasecmp(temp, "Resolution"))
	{
		infile >> mPlotResolution;
		infile >> temp;
	}
	if (!strcasecmp(temp, "MinAngleRes"))
	{
		infile >> mMinAngleRes;
		infile >> temp;
	}
	if (!strcasecmp(temp, "Mobile"))
	{
		infile >> mMobile.sInstKey;
		infile >> temp;
	}
	if (!strcasecmp(temp, "NumFixed"))
	{
		infile >> mNumFixed;
		infile >> temp;

		if ((strcasecmp(temp, "Fixed"))||(mNumFixed==0))
		{
			string err = "Error reading file: ";
			err+=filename;
			err+=" at Fixed Installations: There must be at least one fixed installation.";
			QRAP_WARN(err.c_str());
			return false;
		}
		else 
		{
			// Clear the fixed intallations vector
			mFixedInsts.clear();
			mMaxRange=0;
			// Get the installations primary keys and ranges
			for (i=0; i<mNumFixed; i++)
			{
				tFixed tempInst;
				
				infile >> tempInst.sInstKey;
				infile >> tempInst.sRange;
				tempInst.sRange *= 1000;
				tempInst.sCentroidX = 0;
				tempInst.sCentroidY = 0;
				tempInst.sPixelCount = 0;
				if (tempInst.sRange>mMaxRange) 
					mMaxRange = tempInst.sRange;
				mFixedInsts.push_back(tempInst);
			}
			if (mMaxRange>0)
			{
				if (mMinAngleRes<(270*mPlotResolution/mMaxRange/PI))
				{
					if (mPlotResolution<0.5) mPlotResolution=5; 
					int mNumAngles = (int)ceil(1.5*PI*mMaxRange/mPlotResolution);
					mMinAngleRes=360.0/(double)mNumAngles;
				}
			}
			if (mMinAngleRes<0.0001)
			{
				mMinAngleRes=0.2;
				mNumAngles =1800;
			}
			mNumAngles = (unsigned)(360.0/mMinAngleRes);
			if (mNumAngles<4)
			{
				mMinAngleRes = 10;
				mNumAngles = 36;
			}
			infile >> temp;
		}
	}
	
	
	if (!strcasecmp(temp, "Area"))
	{
		double N, W, S, E;
		infile >> temp;
		for (i=0;i<4;i++)
		{
			if (!strcasecmp(temp, "N"))
				infile >> N;
			else if (!strcasecmp(temp, "S"))
				infile >> S;
			else if (!strcasecmp(temp, "E"))
				infile >> E;
			else if (!strcasecmp(temp, "W"))
				infile >> W;
			infile >> temp;
		}
		mNorthWest.Set(N, W, DEG);
		mSouthWest.Set(S, W, DEG);
		mSouthEast.Set(S, E, DEG);
		mNorthEast.Set(N, E, DEG);

		double Lat,Lon;
		char* temp2;
		temp2 = new char[33];
		string PointString;
		unsigned spacePos; 
		pqxx::result SiteLoc;
		for (unsigned ii=0; ii<mFixedInsts.size(); ii++)
		{
			gcvt(mFixedInsts[ii].sInstKey,9,temp2);
			string query = "SELECT ST_AsText(location) AS location"; 
			query += " FROM radioinstallation cross join site WHERE ";
			query += " siteid =site.id AND ";
			query += "radioinstallation.id  = ";
			query += temp2;	
			if (!gDb.PerformRawSql(query))
			{
				cout << "Database Select on sites table failed"<< endl;
			}
			gDb.GetLastResult(SiteLoc);
			if (SiteLoc.size() >0)
			{	
				PointString = SiteLoc[0]["location"].c_str();
				spacePos = PointString.find_first_of(' ');
				Lon = atof((PointString.substr(6,spacePos).c_str())); 
				Lat = atof((PointString.substr(spacePos,PointString.length()-1)).c_str());
				mFixedInsts[ii].sSitePos.Set(Lat,Lon,DEG);
				cGeoP Temp;
				Temp.Set(Lat,Lon,DEG);
				N = max(N,Lat);
				S = min(S,Lat);
				E = max(E,Lon);
				W = min(W,Lon);
//				cout << "2	N: " << N << "	S: "<< S << "	E: " << E << "	W: " << W << endl;
				cGeoP NewPoint;
//				mFixedInsts[ii].sSitePos.Display();
//				cout << mFixedInsts[ii].sRange << endl;
				NewPoint.FromHere(mFixedInsts[ii].sSitePos,mFixedInsts[ii].sRange,0.0);
//				NewPoint.Display();
				//Check North
				NewPoint.Get(Lat,Lon);
				N = max(N,Lat);
//				cout << "3	N: " << N << "	S: "<< S << "	E: " << E << "	W: " << W << endl;
				//Check East
				NewPoint.FromHere(mFixedInsts[ii].sSitePos,mFixedInsts[ii].sRange,90.0);
				NewPoint.Get(Lat,Lon);
				E = max(E,Lon);
//				cout << "4	N: " << N << "	S: "<< S << "	E: " << E << "	W: " << W << endl;
				//Check South
				NewPoint.FromHere(mFixedInsts[ii].sSitePos,mFixedInsts[ii].sRange,180.0);
				NewPoint.Get(Lat,Lon);
				S = min(S,Lat);
//				cout << "5	N: " << N << "	S: "<< S << "	E: " << E << "	W: " << W << endl;
				//Check West
				NewPoint.FromHere(mFixedInsts[ii].sSitePos,mFixedInsts[ii].sRange,270.0);
				NewPoint.Get(Lat,Lon);
				W = min(W,Lon);
//				cout << "6	N: " << N << "	S: "<< S << "	E: " << E << "	W: " << W << endl;
			}
		}
		mNorthWest.Set(N, W, DEG);
		mSouthWest.Set(S, W, DEG);
		mSouthEast.Set(S, E, DEG);
		mNorthEast.Set(N, E, DEG);
				
//		double WE = max(mNorthWest.Distance(mNorthEast), 
//						mSouthWest.Distance(mSouthEast));
//		mRows = (int)ceil(mNorthWest.Distance(mSouthWest)/mPlotResolution);
//		mCols = (int)ceil(WE/mPlotResolution);
		mNSres = mPlotResolution*cDegResT;
		mEWres = mPlotResolution*cDegResT;
		mRows = (unsigned)(2.0*ceil((N-S)/mNSres/2.0)+1.0);
		mCols = (unsigned)(2.0*ceil((E-W)/mEWres/2.0)+1.0);
		delete_Float2DArray(mPlot);
		delete_Float2DArray(mSupportPlot);
		mPlot = new_Float2DArray(mRows,mCols);
		mSupportPlot = new_Float2DArray(mRows,mCols);
		cGeoP MidNorth(N,(W+E)/2.0, DEG);
		cGeoP Mid((N+S)/2.0,(E+W)/2.0,DEG);
		mNorthWest.Set((N+S)/2.0+(double)mRows*mNSres/2.0, (E+W)/2.0-(double)mCols*mEWres/2.0,DEG);
		mSouthWest.Set((N+S)/2.0-(double)mRows*mNSres/2.0, (E+W)/2.0-(double)mCols*mEWres/2.0,DEG);
		mNorthEast.Set((N+S)/2.0+(double)mRows*mNSres/2.0, (E+W)/2.0+(double)mCols*mEWres/2.0,DEG);
		mSouthEast.Set((N+S)/2.0-(double)mRows*mNSres/2.0, (E+W)/2.0+(double)mCols*mEWres/2.0,DEG);
		mCentMer = Mid.DefaultCentMer(WGS84GC);
//		mNorthWest.FromHere(MidNorth,mCols/2.0*mPlotResolution,270.0);
		double dummy;
		if (mCols<mRows)
			mNorthWest.Get(mCurrentEdge, dummy);
		else 
			mNorthWest.Get(dummy, mCurrentEdge);
		delete [] temp2;
	}
	if (!strcasecmp(temp, "DEM"))
	{
		infile >> mDEMsource;
		FoundRasterSet = mDEM.SetRasterFileRules(mDEMsource);
		if (!FoundRasterSet)
		{
			string err = "Error reading file: ";
			err+=filename;
			err+=" at DEM: Trouble getting DEM list. Using default";
			QRAP_WARN(err.c_str());
			//return false;
		}
		infile >> temp;
	}
	if (!strcasecmp(temp, "UseClutter"))
	{
		infile >> temp;
		if (!strcasecmp(temp, "TRUE"))
			mUseClutter=true;
		else mUseClutter=false;
		infile >> temp;
	}
	if (!strcasecmp(temp, "Clutter"))
	{
		infile >> mClutterSource;
		FoundRasterSet = mClutter.SetRasterFileRules(mClutterSource);
		mUseClutter = FoundRasterSet;
		if (!FoundRasterSet)
		{
			string err = "Error reading file: ";
			err += filename;
			err+=" at Clutter: Trouble getting Clutter list. Using default.";
			QRAP_WARN(err.c_str());
		}
	}
	else mUseClutter = false;
	if (!strcasecmp(temp, "OutputDir"))
		infile >> mDir;
	infile >> temp;
	if (!strcasecmp(temp, "OutputFile"))
		infile >> mOutputFile;
	bool ReturnValue = GetDBinfo();
	delete [] temp;
	return ReturnValue;
}// end cPlotTask::ReadPlotRequest

	
//***************************************************************************
bool cPlotTask::CombineCov()
{
	// Some temporary variables
	unsigned i,j,k,Prows,Pcols;
	int ki,kj;
	unsigned Advance = 10;
	cout << "In cPlotTask::CombineCov()" << endl;
	string err = "Starting requested Prediction.";
	cout << err<< endl;
//	QRAP_INFO(err.c_str());
	
	Float2DArray Test2;
	Float2DArray Inst;
    Float2DArray mPlot2;
    mPlot2 = new_Float2DArray(mRows,mCols);

    if (mCols<mRows) //! 经度上的栅格数小于纬度上的栅格数，呈'高大于宽'的数组
		cout << "vertical" << endl;
	else cout << "horisontal" << endl;
	
	if (mPlotType==SecondServer) //! skip
	{
		Inst = new_Float2DArray(mRows,mCols);
		Test2 = new_Float2DArray(mRows,mCols);
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
			{
				Test2[i][j]=-9999;
				Inst[i][j]=0;
			}
	}
	
	for (i=0;i<mRows;i++)
		for (j=0;j<mCols;j++)
			mSupportPlot[i][j]=-9999; //!初始值
	
	if ((mPlotType==NumServers)||(mPlotType==NumInt)) //! skip
	{
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
				mPlot[i][j]=0;
	}
	else //! Cov, enter
	{
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
            {
				mPlot[i][j]=-9999; //! 初始值
                mPlot2[i][j]=-9999;
            }
	}	

	cout << "cPlotTask::CombineCov():  Before Order array;" << endl;
	if (OrderAllPred()==0) //! 处理 重点1
		return false;
	
	cout << "cPlotTask::CombineCov():  First UpdateActiveRasters;" << endl;
	double RangeSum=0;
	for (k=0; k<mFixedInsts.size(); k++)
		RangeSum += mFixedInsts[k].sRange;
	Advance = RangeSum/mFixedInsts.size(); //! 范围均值 单位为米
	
	UpdateActiveRasters(0,Advance+2); //! 重点2
/*	for (k=0;k<mActiveRasters.size();k++)
	{
		cout << "	Top: " << mActiveRasters[k].sTop;
		cout << "	Left: " << mActiveRasters[k].sLeft;
		cout << endl;
	}
*/	
	cout << "cPlotTask::CombineCov():  Before main loop;" << endl;	
	if(mCols<mRows)  //! if vertical. 纬度栅格更多，行数多 //重点3
	{
		for(i=0; i<mRows; i++)
		{
			for (j=0; j<mCols; j++)
			{
				for (k=0;k<mActiveRasters.size();k++) //!每个的覆盖图层
				{
					ki = i - mActiveRasters[k].sTop;
					kj = j - mActiveRasters[k].sLeft;
					Prows = mActiveRasters[k].sNSsize;
					Pcols = mActiveRasters[k].sEWsize;
//					cout << Prows << "  P RxC " << Pcols <<endl; 
					if ((ki>=0)&&(kj>=0)&&(ki<Prows)&&(kj<Pcols))
					{
						switch (mPlotType)
						{
							case Cov: //!
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]= max(mPlot[i][j],mActiveRasters[k].sRaster[ki][kj]);
                                    mPlot2[i][j] = mPlot[i][j];
								break;
							case NumServers:
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]++;
								break;
							case PrimServer:
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case CellCentroid: //Same as Primary Server
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case TrafficDist: // Same as Primary Server
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case SecondServer:
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
										&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mPlot[i][j] = Inst[i][j];
									Inst[i][j] = mActiveRasters[k].sInstKey;
									mSupportPlot[i][j]= Test2[i][j];
									Test2[i][j] = mActiveRasters[k].sRaster[ki][kj];
								}
								break;
							case SN:
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]= max(mPlot[i][j],
										(mActiveRasters[k].sRaster[ki][kj]-(float)mNoiseLevel));
								break;
							default: mPlot[i][j]= max(mPlot[i][j],mActiveRasters[k].sRaster[ki][kj]);
						}//end switch
					}//end if ki
				}//end for k
			}//end for j
			if (floor(i/Advance)==(((double)i)/(double)Advance))
				UpdateActiveRasters(i,Advance+2);
		}//end for i
	} // end if vertical
	else //!horisontal
	{
		for(j=0; j<mCols; j++)
		{
			for (i=0; i<mRows; i++)
			{
				for (k=0;k<mActiveRasters.size();k++)
				{
					ki = i - mActiveRasters[k].sTop;
					kj = j - mActiveRasters[k].sLeft;
					Prows = mActiveRasters[k].sNSsize;
					Pcols = mActiveRasters[k].sEWsize;
					if ((ki>=0)&&(kj>=0)&&(ki<Prows)&&(kj<Pcols))
					{
						switch (mPlotType)
						{
							case Cov:
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]= max(mPlot[i][j],mActiveRasters[k].sRaster[ki][kj]);
                                    mPlot2[i][j] = mPlot[i][j];
								break;
							case NumServers:
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]++;
								break;
							case PrimServer:
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case CellCentroid: // Same as primary server
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case TrafficDist: //Same as primary server
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mSupportPlot[i][j]=mActiveRasters[k].sRaster[ki][kj];
									mPlot[i][j]=mActiveRasters[k].sInstKey;
								}
								break;
							case SecondServer:
								if ((mActiveRasters[k].sRaster[ki][kj]>mRxMin)
										&&(mActiveRasters[k].sRaster[ki][kj]>mSupportPlot[i][j]))
								{
									mPlot[i][j] = Inst[i][j];
									Inst[i][j] = mActiveRasters[k].sInstKey;
									mSupportPlot[i][j] = Test2[i][j];
									Test2[i][j] = mActiveRasters[k].sRaster[ki][kj];
								}
								break;
							case SN:
								if (mActiveRasters[k].sRaster[ki][kj]>mRxMin)
									mPlot[i][j]= max(mPlot[i][j],
										(mActiveRasters[k].sRaster[ki][kj]-(float)mNoiseLevel));
								break;
							default: mPlot[i][j]= max(mPlot[i][j],mActiveRasters[k].sRaster[ki][kj]);
						} // end switch
					} //end if ki
				} //end for k
			} // end for i
			if (floor(j/Advance)==(((double)j)/(double)Advance))
				UpdateActiveRasters(j,Advance+2);
/*			cout << "j: " << j;
			for(i=0;i<mRows;i++)
				cout << "  " << mPlot[i][j];
			cout << endl;
*/		}//end for j
	}//! end horisontal
	
    // added by wxn
    for(i=0;i<mRows;i++)
    {
        for(j=0;j<mCols;j++)
        {
            int sum=0,c=0;
            for(unsigned k=max((unsigned)0,i-1);k<=min(i+1,mRows-1);k++)
            {
                for(unsigned u=max((unsigned)0,j-1); u<=min(j+1,mCols-1);u++)
                {
                    sum+=mPlot2[k][u];
                    c++;
                }
            }
            mPlot[i][j]=(float)sum/c;
        }
    }
    delete_Float2DArray(mPlot2);

    //-----------------------------------


	cout << " Almost end of cPlotTask::CombineCov()" << endl;
	if (mPlotType!=Cov)
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
				if (mPlot[i][j]==0)
					mPlot[i][j]=-9999;
	
//	Calculate % coverage.
	int CoveredPixels = mRows*mCols;
	for (i=0;i<mRows;i++)
		for (j=0;j<mCols;j++)
			if (mPlot[i][j]<-200)
				CoveredPixels--;

    double PercCoverage = 100*CoveredPixels/ (mRows*mCols);

	if (mPlotType==SecondServer)
	{
		delete_Float2DArray(Test2);
		delete_Float2DArray(Inst);
	}
	cout << " Exiting cPlotTask::CombineCov()" << endl;
	err = "End of Prediction: Percentage Coverage = ";
	char *temp;
	temp = new char[30];
	gcvt(PercCoverage,8,temp);
	err += temp;
	cout << err << endl;
	QRAP_INFO(err.c_str());
	delete [] temp;
	return true;
}

//***************************************************************************
bool cPlotTask::InterferencePlot()
{
	ePlotType StorePlotType = mPlotType;
	mPlotType=PrimServer;
	CombineCov();
	mPlotType = StorePlotType;
	
	GetDBIntInfo();
	// Some temporary variables
	unsigned i,j,k,Prows,Pcols;
	int ki,kj;
	unsigned Advance = 10;
//	cout << "In cPlotTask::CombineCov()" << endl;
	string err = "Starting requested Interference Prediction.";
	cout << err<< endl;
//	QRAP_INFO(err.c_str());
	
	Float2DArray Test2;
	if ((mPlotType==PrimIntAd)||(mPlotType==PrimIntCo))
	{
		Test2 = new_Float2DArray(mRows,mCols);
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
				Test2[i][j]=9999;
	}		

	cout << "cPlotTask::InterferencePlot():  Before Order array;" << endl;
	if (OrderAllPred()==0)
		return false;
	
	cout << "cPlotTask::InterferencePlot():  First UpdateActiveRasters;" << endl;
	double RangeSum=0;
	for (k=0; k<mFixedInsts.size(); k++)
		RangeSum += mFixedInsts[k].sRange;
	Advance = RangeSum/mFixedInsts.size(); 
	UpdateActiveRasters(0,Advance+2);

/*	for (k=0;k<mActiveRasters.size();k++)
	{
		cout << "	Top: " << mActiveRasters[k].sTop;
		cout << "	Left: " << mActiveRasters[k].sLeft;
		cout << endl;
	}
*/
	tFixed CurrentServer;
	bool CoInterfering=false;
	bool AdInterfering=false;
	float hBW, IntLevel;
	CurrentServer.sInstKey = -9999;
	unsigned ii=0, jj;
	unsigned size = mFixedInsts.size();
	
	if(mCols<mRows)  //vertical
	{
		for(i=0; i<mRows; i++)
		{
			for (j=0; j<mCols; j++)
			{
				if (mPlot[i][j]!=-9999)
				{
					if (CurrentServer.sInstKey!=mPlot[i][j])
					{
						ii=0;
						while ((ii<size)&&(mPlot[i][j]!=mFixedInsts[ii].sInstKey))
							ii++;
						CopyFixed(CurrentServer,mFixedInsts[ii]);
					}
					if ((mPlotType==IntRatioCo)||(mPlotType==IntRatioAd))
						mPlot[i][j]=9999;
					else mPlot[i][j]=0; 
				
					for (k=0;k<mActiveRasters.size();k++)
					{
						ki = i - mActiveRasters[k].sTop;
						kj = j - mActiveRasters[k].sLeft;
						Prows = mActiveRasters[k].sNSsize;
						Pcols = mActiveRasters[k].sEWsize;
//						cout << Prows << "  P RxC " << Pcols <<endl; 
						if ((ki>=0)&&(kj>=0)&&(ki<Prows)&&(kj<Pcols)
								&&(mActiveRasters[k].sInstKey!=CurrentServer.sInstKey))
						{
							CoInterfering = false;
							AdInterfering = false;
							hBW = max(mActiveRasters[k].sBandWidth, CurrentServer.sBandWidth)/2.0;
							cout << "ServerNumFreq: "<< CurrentServer.sFreqList.size() << endl;
							cout << "RasterNumFreq: "<< mActiveRasters[k].sFreqList.size() << endl;
							for (ii=0; ii<CurrentServer.sFreqList.size(); ii++ )
							{
								for (jj=0;jj<mActiveRasters[k].sFreqList.size(); jj++)
								{
									CoInterfering = CoInterfering||
										(fabs(mActiveRasters[k].sFreqList[jj]-CurrentServer.sFreqList[ii])<hBW);
									AdInterfering = AdInterfering||
										(fabs(CurrentServer.sFreqList[ii]-CurrentServer.sBandWidth-mActiveRasters[k].sFreqList[jj])<hBW)
										||(fabs(CurrentServer.sFreqList[ii]+CurrentServer.sBandWidth-mActiveRasters[k].sFreqList[jj])<hBW);
								}
							}
							if (CoInterfering)
							{
								IntLevel= mSupportPlot[i][j]-mActiveRasters[k].sRaster[ki][kj];
								if (IntLevel<mRqCIco)
								{
									if (mPlotType==IntRatioCo)
										mPlot[i][j]= min(mPlot[i][j], IntLevel);
									else if (mPlotType==PrimIntCo)
									{
										if (IntLevel<Test2[i][j])
										{
											Test2[i][j]=IntLevel;
											mPlot[i][j]=mActiveRasters[k].sInstKey;
										}
									}
									else if (mPlotType==IntAreas)
										mPlot[i][j]=2;
									else if (mPlotType==NumInt)
										mPlot[i][j]+=1;
								} // end if CoInt above threshold
							}// end if Cochannel int
							else if (AdInterfering)
							{
								IntLevel= mSupportPlot[i][j]-mActiveRasters[k].sRaster[ki][kj];
								if (IntLevel<mRqCIad)
								{
									if (mPlotType==IntRatioAd)
										mPlot[i][j]= min(mPlot[i][j], IntLevel);
									else if (mPlotType==PrimIntAd)
									{
										if (IntLevel<Test2[i][j])
										{
											Test2[i][j]=IntLevel;
											mPlot[i][j]=mActiveRasters[k].sInstKey;
										}
									}
									else if ((mPlotType==IntAreas)&&(mPlot[i][j]==0))
										mPlot[i][j]=1;
									else if (mPlotType==NumInt)
										mPlot[i][j]+=1;
								} // end if AdInt above threshold									
							}//end if Adjacent chennel
						}//end if ki
					}//end for k
				}//end if (mPlot!=-9999) 
			}//end for j
			if (floor(i/Advance)==(((double)i)/(double)Advance))
				UpdateActiveRasters(i,Advance+2);
		}//end for i
	} // end if vertical
	else //horisontal
	{
		for(i=0; i<mRows; i++)
		{
			for (j=0; j<mCols; j++)
			{
				if (mPlot[i][j]!=-9999)
				{
//					cout << "mPlot[" << i << "][" << j << "]: " << mPlot[i][j] << endl; 
					if (CurrentServer.sInstKey!=mPlot[i][j])
					{
						ii=0;
						while ((ii<size)&&(mPlot[i][j]!=mFixedInsts[ii].sInstKey))
						{
//							cout << ii << "    key: " << mFixedInsts[ii].sInstKey << endl;
							ii++;
						}
						if (ii<size) CopyFixed(CurrentServer,mFixedInsts[ii]);
						else CurrentServer.sInstKey=-9999;
					}
					
					if ((mPlotType==IntRatioCo)||(mPlotType==IntRatioAd))
						mPlot[i][j]=9999;
					else mPlot[i][j]=0; 
					if (CurrentServer.sInstKey!=-9999)
					{
						for (k=0;k<mActiveRasters.size();k++)
						{
							ki = i - mActiveRasters[k].sTop;
							kj = j - mActiveRasters[k].sLeft;
							Prows = mActiveRasters[k].sNSsize;
							Pcols = mActiveRasters[k].sEWsize;
//							cout << Prows << "  P RxC " << Pcols <<endl; 
							if ((ki>=0)&&(kj>=0)&&(ki<Prows)&&(kj<Pcols)
								&&(mActiveRasters[k].sInstKey!=CurrentServer.sInstKey))
							{
								CoInterfering = false;
								AdInterfering = false;
								hBW = max(mActiveRasters[k].sBandWidth, CurrentServer.sBandWidth)/2.0;
								for (ii=0; ii<CurrentServer.sFreqList.size(); ii++ )
								{
									for (jj=0;jj<mActiveRasters[k].sFreqList.size(); jj++)
									{
//										cout << CurrentServer.sFreqList[ii] << " ff " << mActiveRasters[k].sFreqList[jj] << endl;
										CoInterfering = CoInterfering||
											(fabs(mActiveRasters[k].sFreqList[jj]-CurrentServer.sFreqList[ii])<hBW);
										AdInterfering = AdInterfering||
											(fabs(CurrentServer.sFreqList[ii]-CurrentServer.sBandWidth-mActiveRasters[k].sFreqList[jj])<hBW)
											||(fabs(CurrentServer.sFreqList[ii]+CurrentServer.sBandWidth-mActiveRasters[k].sFreqList[jj])<hBW);
									}
								}
								if (CoInterfering)
								{
									IntLevel= mSupportPlot[i][j]-mActiveRasters[k].sRaster[ki][kj];
									if (IntLevel<mRqCIco)
									{
										if (mPlotType==IntRatioCo)
											mPlot[i][j]= min(mPlot[i][j], IntLevel);
										else if (mPlotType==PrimIntCo)
										{
											if (IntLevel<Test2[i][j])
											{
												Test2[i][j]=IntLevel;
												mPlot[i][j]=mActiveRasters[k].sInstKey;
											}
										}
										else if (mPlotType==IntAreas)
											mPlot[i][j]=2;
										else if (mPlotType==NumInt)
											mPlot[i][j]+=1;
									} // end if CoInt above threshold
								}// end if Cochannel int
								else if (AdInterfering)
								{
									IntLevel= mSupportPlot[i][j]-mActiveRasters[k].sRaster[ki][kj];
									if (IntLevel<mRqCIad)
									{
										if (mPlotType==IntRatioAd)
											mPlot[i][j]= min(mPlot[i][j], IntLevel);
										else if (mPlotType==PrimIntAd)
										{
											if (IntLevel<Test2[i][j])
											{
												Test2[i][j]=IntLevel;
												mPlot[i][j]=mActiveRasters[k].sInstKey;
											}
										}
										else if ((mPlotType==IntAreas)&&(mPlot[i][j]==0))
											mPlot[i][j]=1;
										else if (mPlotType==NumInt)
											mPlot[i][j]+=1;
									} // end if AdInt above threshold									
								}//end if Adjacent chennel
							}//end if ki
						}//end for k
					} // if currentserver!=-9999
				}//end if (mPlot!=-9999) 
			}//end for j
			if (floor(i/Advance)==(((double)i)/(double)Advance))
				UpdateActiveRasters(i,Advance+2);
		}//end for i
	}// end horisontal
	
	cout << " Almost end of cPlotTask::InterferencePlot()" << endl;
	if ((mPlotType!=IntRatioCo)&&(mPlotType!=IntRatioAd))
	{
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
				if (mPlot[i][j]==0)
					mPlot[i][j]=-9999;
	}
	else
	{
		for (i=0;i<mRows;i++)
			for (j=0;j<mCols;j++)
				if (mPlot[i][j]==9999)
					mPlot[i][j]=-9999;
	}
	
	int CoveredPixels = mRows*mCols;
	for (i=0;i<mRows;i++)
		for (j=0;j<mCols;j++)
			if (mPlot[i][j]<-200)
				CoveredPixels--;

	double PercCoverage = 100*(double)CoveredPixels/ (double)(mRows*mCols);

	if ((mPlotType==PrimIntAd)||(mPlotType==PrimIntCo))
		delete_Float2DArray(Test2);
	cout << " Exiting cPlotTask::InterferencePlot()" << endl;
	err = "End of Interference Prediction: Percentage Coverage = ";
	char *temp;
	temp = new char[30];
	gcvt(PercCoverage,8,temp);
	err += temp;
	cout << err << endl;
	QRAP_INFO(err.c_str());
	return true;
}

//*****************************************************************************
unsigned cPlotTask::UpdateActiveRasters(int Here, int Advance)
{
	unsigned i ,j;
	cGeoP Marker;
	double FrontEdge,DoneEdge,dummy,N,W,gLat,gLon;
	Marker.SetGeoType(DEG);
	mNorthWest.SetGeoType(DEG);
	mNorthWest.Get(gLat,gLon); //! 西北位置的经纬度 给了gLat，gLon 和 N，W
	mNorthWest.Get(N,W);
	
	cout<<"mActiveRasters.size(): "<<mActiveRasters.size()<<endl; //首次时为0

	if (mCols<mRows) //! vertical
	{	
		for (i=0; i<mActiveRasters.size(); i++) //!首次时不进入该循环
		{
			if ((mActiveRasters[i].sTop+mActiveRasters[i].sNSsize)<(Here-2))
			{
				cout << endl <<"cPlotTask::UpdateActiveRasters. ";
				cout << "Deleting ActiveRaster of Inst: " << mActiveRasters[i].sInstKey << endl;
				cout << " Here = " << Here << endl << endl;
				delete_Float2DArray(mActiveRasters[i].sRaster);
				mActiveRasters.erase(mActiveRasters.begin()+i);
			}
		}
//		Marker.FromHere(mNorthWest,(Here-1)*mPlotResolution,180);
//		Marker.Get(DoneEdge,dummy);
//		Marker.FromHere(mNorthWest,(Here+Advance+1)*mPlotResolution,180);
//		Marker.Get(FrontEdge,dummy);
		DoneEdge = N - (Here)*mPlotResolution*0.00083333/90.0; //! here 为0 时， 此值为N
		FrontEdge = N - (Here+Advance+1)*mPlotResolution*0.00083333/90.0; //! 向南一段距离，单位为纬度Degree
	}
    else //! horisontal
	{
		for (i=0; i<mActiveRasters.size(); i++)
		{
			if ((mActiveRasters[i].sLeft+mActiveRasters[i].sEWsize)<(Here-2))
			{
				cout << endl << "cPlotTask::UpdateActiveRasters. ";
				cout << "Deleting ActiveRaster of Inst: " << mActiveRasters[i].sInstKey << endl;
				cout << " Here = " << Here << endl << endl;
				delete_Float2DArray(mActiveRasters[i].sRaster);
				mActiveRasters.erase(mActiveRasters.begin()+i);
			}
		}
//		Marker.FromHere(mNorthWest,(Here-1)*mPlotResolution,90);
//		Marker.Get(dummy,DoneEdge);
//		Marker.FromHere(mNorthWest,(Here+Advance+1)*mPlotResolution,90);
//		Marker.Get(dummy,FrontEdge);
		DoneEdge = W + (Here)*mPlotResolution*0.00083333/90.0;
		FrontEdge = W + (Here+Advance+1)*mPlotResolution*0.00083333/90.0;
	}
	
	bool Loaded=false;
	cGeoP Corner, Temp;
	double pLat, pLon;
	cCoveragePredict Prediction; //! 此处又一个prediction
	tSignalRaster newRaster; //! 栅格对象
	int FixedAntPatternKey, MobileAntPatternKey;
	double FixedAzimuth, FixedMechTilt;
	double EIRP, TxPower, TxSysLoss, RxSysLoss, RxSens;
	MobileAntPatternKey = mMobile.sPatternKey;
	double MobileHeight = mMobile.sMobileHeight;
	double FixedHeight;
	unsigned tempNumAngles, tempNumDist;
	double tempPlotRes, tempAngRes;
	string query, err;
	char dummyS[33], numSiteStr[33];
	int BTLkey;
	bool AfterReceiver = (mUnits==dBW)||(mUnits==dBm)||(mUnits==dBuV); //! true
	bool CovOrInt = ((mPlotType==Cov)||(mPlotType==PrimServer)||(mPlotType==CellCentroid)||
			(mPlotType==TrafficDist)||(mPlotType==SecondServer)||(mPlotType==NumServers));
	double kFactor = 1.33;
	if (CovOrInt)
		kFactor = mkFactorServ; //! 值被替换
	else kFactor = mkFactorInt;
    gcvt(mFixedInsts.size(),8,numSiteStr); //! float转换为字符串
	mNorthWest.Get(gLat,gLon); //! 西北角的经纬度

	cout << "cPlotTask::UpdateActiveRasters. ";
	cout << "FrontEdge = " << FrontEdge << "	DoneEdge = " <<  DoneEdge << endl;

    //----------------------------------
    //! loop for each sector
    //!
	for (i=0; i<mFixedInsts.size(); i++)
	{
		if (((mCols<mRows)&&((mFixedInsts[i].sFEdge>=FrontEdge)
						&&(mFixedInsts[i].sBEdge<=DoneEdge)))
			||((mCols>=mRows)&&((mFixedInsts[i].sFEdge<=FrontEdge)
							&&(mFixedInsts[i].sBEdge>=DoneEdge))))
		{
			cout << "i: " << i << "  InstKey: " << mFixedInsts[i].sInstKey 	
							<< " FE: " << mFixedInsts[i].sFEdge 
							<< "   BE: " <<  mFixedInsts[i].sBEdge << endl;
			Loaded = false;
			for (j=0;j<mActiveRasters.size();j++)
				Loaded = Loaded || (mActiveRasters[j].sInstKey==mFixedInsts[i].sInstKey); //! 判断是否已经加载
			if (!Loaded)
			{
				
				mInstCounter++;
                if (mDownlink) //here
				{
					EIRP = mFixedInsts[i].sEIRP;
					TxPower = mFixedInsts[i].sTxPower;
					TxSysLoss = mFixedInsts[i].sTxSysLoss;
					FixedAntPatternKey = mFixedInsts[i].sTxPatternKey;
					FixedAzimuth = mFixedInsts[i].sTxAzimuth;
					FixedMechTilt = mFixedInsts[i].sTxMechTilt;
					FixedHeight = mFixedInsts[i].sTxHeight;
					RxSysLoss = mMobile.sRxSysLoss;
					RxSens = mMobile.sRxSens;		
				}
				else
				{
					EIRP = mMobile.sEIRP;
					TxPower = mMobile.sTxPower;
					TxSysLoss = mMobile.sTxSysLoss;
					FixedAntPatternKey = mFixedInsts[i].sRxPatternKey;
					FixedAzimuth = mFixedInsts[i].sRxAzimuth;
					FixedMechTilt = mFixedInsts[i].sRxMechTilt;
					FixedHeight = mFixedInsts[i].sRxHeight;
					RxSysLoss = mFixedInsts[i].sRxSysLoss;
					RxSens = mFixedInsts[i].sRxSens;
				}
				if (!CovOrInt) mFixedInsts[i].sRange=mMaxRange; 
                //! 读取和设置一些值  	cCoveragePredict Prediction;
				BTLkey = Prediction.SetCommunicationLink(mFixedInsts[i].sSiteID,
										mDownlink, EIRP, TxPower, TxSysLoss, 
										RxSysLoss, RxSens,
										mFixedInsts[i].sInstKey, FixedAzimuth, FixedMechTilt,
										mMobile.sInstKey, FixedHeight, MobileHeight, 
										mFixedInsts[i].sFrequency, kFactor,
										mFixedInsts[i].sRange, mPlotResolution, 
										mNumAngles, mDEMsource, mUseClutter, mClutterSource);

				cout << " In cPlotTask::UpdateActiveRasters. BTLkey = " << BTLkey << endl;

				if (BTLkey==-1)
				{	
//					err = "Loading DEM (and Clutter) Data for Site: ";
					err = "Performing Basic Transmission Loss Calculations for Site: ";
					gcvt(mFixedInsts[i].sSiteID,8,dummyS);
					err+=dummyS;
					err+=" for installation ";
					gcvt(mFixedInsts[i].sInstKey,8,dummyS);
					err+=dummyS;
					err+=" which is ";
					gcvt(mInstCounter,8,dummyS);
					err+=dummyS;
					err+= " of ";
					err+= numSiteStr;
					cout << err << endl;
//					QRAP_INFO(err.c_str());

					tempPlotRes = mPlotResolution;
					tempNumAngles = mNumAngles;
					tempAngRes = 360.0/tempNumAngles;
					tempNumDist = (int)(mFixedInsts[i].sRange/tempPlotRes);
					Float2DArray DTM;
					DTM = new_Float2DArray(tempNumAngles,tempNumDist); //! 数组 （角度数，距离点数）
					Float2DArray Clutter;
					cout << "NA: " << tempNumAngles << "	ND: " << tempNumDist << endl; 

					if (mUseClutter) //! skip
						Clutter = new_Float2DArray(tempNumAngles,tempNumDist);

                    //!------------------------------------------------------------
                    //!	vaiable DTM 中each value包含了每个基站周边的高度
                    //! cRasterFileHandler 	mDEM;
					mDEM.GetForCoverage(false, mFixedInsts[i].sSitePos, 
									mFixedInsts[i].sRange, tempPlotRes, tempAngRes,
									tempNumAngles, tempNumDist, DTM);
/*					for (unsigned ii = 0; ii< tempNumAngles; ii++)
					{
						for (unsigned jj=0; jj<tempNumDist; jj++)
							cout << DTM[ii][jj] << "	";
						cout << endl;
					}
*/	
					if (mUseClutter)//! skip
					{
						mClutter.GetForCoverage(true, mFixedInsts[i].sSitePos, 
										mFixedInsts[i].sRange, tempPlotRes, tempAngRes,
										tempNumAngles, tempNumDist, Clutter);
					}
					err = "Performing Basic Transmission Loss Calculations for Site: ";
					gcvt(mFixedInsts[i].sSiteID,8,dummyS);
					err+=dummyS;
					err+=" for installation ";
					gcvt(mFixedInsts[i].sInstKey,8,dummyS);
					err+=dummyS;
					err+=" which is ";
					gcvt(mInstCounter,8,dummyS);
					err+=dummyS;
					err+= " of ";
					err+= numSiteStr;
//					QRAP_INFO(err.c_str());
					cout << err << endl;

                    //----------------------------------
                    //
                    //!----------------------------------
					Prediction.mBTLPredict.SetMaxPathLoss(mMaxPathLoss);

					//! 返回storeBTL
                    //! ----------------------------------
					BTLkey = Prediction.mBTLPredict.PredictBTL(tempNumAngles, tempNumDist, tempPlotRes, 
										DTM, mUseClutter, mClutterClassGroup, Clutter);
					mFixedInsts[i].sRange = Prediction.mBTLPredict.GetRange();
//					cout << endl<< endl<< endl<< endl<< endl<< endl<< endl<< "BTL" << endl << endl;
/*					for (unsigned ii = 0; ii< tempNumAngles; ii++)
					{
						for (unsigned jj=0; jj<tempNumDist; jj++)
							cout << Prediction.mBTLPredict.mBTL[ii][jj] << " ";
						cout << endl;
					}
					cout << endl<< endl<< endl<< endl<< endl<< endl<< endl<< "Tilt" << endl << endl;
					for (unsigned ii = 0; ii< tempNumAngles; ii++)
					{
						for (unsigned jj=0; jj<tempNumDist; jj++)
							cout << Prediction.mBTLPredict.mTilt[ii][jj] << " ";
						cout << endl;
					}
*/
					if (mUseClutter) delete_Float2DArray(Clutter);
					delete_Float2DArray(DTM);
				}
				if (BTLkey>0)
				{
					gcvt(BTLkey,8,dummyS);
					query = "update radioinstallation set btlplot=";
					query += dummyS;
					gcvt(mFixedInsts[i].sInstKey,8,dummyS);
					query += " where id=";
					query +=dummyS;
					query +=";";
					cout << " In cPlotTask::UpdateActiveRasters " << endl; 
					cout << query << endl;
					if(!gDb.PerformRawSql(query))
					{
						string err = "Error in Database Updating the RadioInst with BTLkeys. Query: ";
						err += query;
						QRAP_WARN(err.c_str());
					}
				}

				err = "Performing Signal Strength Calculations for Installation ";
				gcvt(mFixedInsts[i].sInstKey,8,dummyS);
				err+=dummyS;
				err+=" which is ";
				gcvt(mInstCounter,8,dummyS);
				err+=dummyS;
				err+= " of ";
				err+= numSiteStr;
//				QRAP_INFO(err.c_str());
				cout << err << endl;


                //--------------------------------------
                //--------------------------------------------
				//!  CalculateRadialCoverage
				Prediction.CalculateRadialCoverage(AfterReceiver);  //! 这里有计算 func(true)

				Temp.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,0.0);
				Temp.SetGeoType(DEG);
				Temp.Get(pLat,dummy);
				Temp.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,270.0);
				Temp.SetGeoType(DEG);
				Temp.Get(dummy,pLon);
				mFixedInsts[i].sSitePos.SetGeoType(DEG);
				mFixedInsts[i].sSitePos.Get(pLat,pLon);
                newRaster.sNSsize = 2*(int)ceil((gLat-pLat)/(mPlotResolution*cDegResT))+1; //!compare with northwest point of the chosen area
                newRaster.sEWsize = 2*(int)ceil((pLon-gLon)/(mPlotResolution*cDegResT))+1;
				pLat += ((double)newRaster.sNSsize * mPlotResolution * cDegResT)/2.0;
				pLon -= ((double)newRaster.sEWsize * mPlotResolution * cDegResT)/2.0;
				newRaster.sTop = (int)((gLat-pLat)/(mPlotResolution*cDegResT));
				newRaster.sLeft = (int)((pLon-gLon)/(mPlotResolution*cDegResT));
				Corner.Set(pLat,pLon,DEG);

//				Temp.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,0);
//				Corner.FromHere(Temp,mFixedInsts[i].sRange,270);
//				Corner.SetGeoType(DEG);
//				Corner.Get(pLat,pLon);
//				Temp.Set(pLat,gLon,DEG);
//				Dist = mNorthWest.Distance(Temp);
//				Bearing = Temp.Bearing(mNorthWest);
//				Dist = cos(Bearing*PI/180.0)*Dist;
//				newRaster.sTop = (int)(Dist/mPlotResolution+0.5);
//				Temp.Set(gLat,pLon,DEG);
//				Dist = mNorthWest.Distance(Temp);
//				Bearing = mNorthWest.Bearing(Temp);
//				Dist = sin(Bearing*PI/180.0)*Dist;
//				newRaster.sLeft = (int)(Dist/mPlotResolution+0.5);
//				if (newRaster.sLeft>0)
//					Temp.FromHere(mNorthWest,newRaster.sLeft*mPlotResolution,90);
//				else Temp.FromHere(mNorthWest,-newRaster.sLeft*mPlotResolution,270);
//				if (newRaster.sTop>0)
//					Corner.FromHere(Temp,newRaster.sTop*mPlotResolution,180);
//				else Corner.FromHere(Temp,-newRaster.sTop*mPlotResolution,0);
//				newRaster.sSize = (int)ceil(2.0*mFixedInsts[i].sRange/mPlotResolution)+2;
				//!
				newRaster.sInstKey = mFixedInsts[i].sInstKey;
				newRaster.sFreqList.clear();
				cout << "NumFreq:  " << mFixedInsts[i].sFreqList.size() << endl;
				for (unsigned ii=0; ii<mFixedInsts[i].sFreqList.size(); ii++ )
				{
					newRaster.sFreqList.push_back(mFixedInsts[i].sFreqList[ii]);
				}
				newRaster.sRaster = new_Float2DArray(newRaster.sNSsize,newRaster.sEWsize);
				//!
				Prediction.InterpolateToSquare(mFixedInsts[i].sSitePos, Corner, newRaster.sRaster, 
							mPlotResolution, newRaster.sNSsize,newRaster.sEWsize);
/*				for (unsigned ii = 0; ii< newRaster.sNSsize; ii++)
				{
					for (unsigned jj=0; jj<newRaster.sEWsize; jj++)
						cout << newRaster.sRaster[ii][jj] << "	";
					cout << endl;
				}
*/				mActiveRasters.push_back(newRaster);
			}
		} //end of if
	} //!各基站循环处理
	cout << "#Rasters: " << mActiveRasters.size() << endl; 
	return mActiveRasters.size();
}

//******************************************************************************
int cPlotTask::OrderAllPred()
{
	unsigned i,j;
	cGeoP Edge;
	double dummy;
	bool swap;
	tFixed temp;
	cCoveragePredict Prediction;//!
	
	int FixedAntPatternKey, MobileAntPatternKey = mMobile.sPatternKey;
	double FixedAzimuth, FixedMechTilt;
	double EIRP, TxPower, TxSysLoss, RxSysLoss, RxSens;
	double MobileHeight = mMobile.sMobileHeight;
	double FixedHeight;
	int PredDone = 3;
	
	cout << "In cPlotTask::OrderAllPred(). " << endl;

	// Where prediction does exist update with the values from the prediction
    for (i=0 ; i<mFixedInsts.size() ; i++) //! 区域内的基站sector数
	{
		if (mDownlink) //! here 基于下行，配置参数
		{
			EIRP = mFixedInsts[i].sEIRP;
			TxPower = mFixedInsts[i].sTxPower;
			TxSysLoss = mFixedInsts[i].sTxSysLoss;
			FixedAntPatternKey = mFixedInsts[i].sTxPatternKey;
			FixedAzimuth = mFixedInsts[i].sTxAzimuth;
			FixedMechTilt = mFixedInsts[i].sTxMechTilt;
			FixedHeight = mFixedInsts[i].sTxHeight;
			RxSysLoss = mMobile.sRxSysLoss;
			RxSens = mMobile.sRxSens;		
		}
		else
		{
			EIRP = mMobile.sEIRP;
			TxPower = mMobile.sTxPower;
			TxSysLoss = mMobile.sTxSysLoss;
			FixedAntPatternKey = mFixedInsts[i].sRxPatternKey;
			FixedAzimuth = mFixedInsts[i].sRxAzimuth;
			FixedMechTilt = mFixedInsts[i].sRxMechTilt;
			FixedHeight = mFixedInsts[i].sRxHeight;
			RxSysLoss = mFixedInsts[i].sRxSysLoss;
			RxSens = mFixedInsts[i].sRxSens;
		}
		
		PredDone = Prediction.SetCommunicationLink(mFixedInsts[i].sSiteID,
								mDownlink, EIRP, 
								TxPower, TxSysLoss, RxSysLoss, RxSens,
								mFixedInsts[i].sInstKey,	FixedAzimuth, 
								FixedMechTilt,	mMobile.sInstKey,
								FixedHeight, MobileHeight, 
                                mFixedInsts[i].sFrequency, mkFactorServ, // 1880
								mFixedInsts[i].sRange, 
								mPlotResolution, mNumAngles, 
								mDEMsource, mUseClutter, mClutterSource);

		cout << "In cPlotTask::OrderAllPred(). in loop i=" << i << endl;
	}

	cout << "In cPlotTask::OrderAllPred(). after loop " << endl;
	
	// Sort to ensure as little as possible 
	// swapping of DEM, BTL and Ant rasters
		
	//Get Northern or Western most point of requested prediction

//    double	mFixedInsts[i].sFEdge;		///< The "front-edge" of the forseen coverage plot in degrees North (or West)
//    double	mFixedInsts[i].sBEdge; 	///< The "back-edge" of the forseen coverage plot in degrees North (or West)
	if (mCols<=mRows) //! vertical
		for (i=0; i<mFixedInsts.size(); i++)
		{
			Edge.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,0); //! 0 表示正北方向？ distance in meter
			Edge.Get(mFixedInsts[i].sFEdge, dummy); //! 此时获取纬度相关
			Edge.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,180);
			Edge.Get(mFixedInsts[i].sBEdge, dummy);
		}
	else
		for (i=0; i<mFixedInsts.size(); i++)
		{
			Edge.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,270);
			Edge.Get(dummy,mFixedInsts[i].sFEdge);
			Edge.FromHere(mFixedInsts[i].sSitePos,mFixedInsts[i].sRange,90);
			Edge.Get(dummy,mFixedInsts[i].sBEdge);
		}
		
	// sort //! 排序
	for (i=0; i<mFixedInsts.size(); i++)
	{
		for (j=i; j<mFixedInsts.size(); j++)
		{
			swap= (((mCols<=mRows)&&(mFixedInsts[i].sFEdge<mFixedInsts[j].sFEdge))
				||((mCols>mRows)&&(mFixedInsts[i].sFEdge>mFixedInsts[j].sFEdge)));
			if (swap)
			{
				CopyFixed(temp,mFixedInsts[i]);
				CopyFixed(mFixedInsts[i],mFixedInsts[j]);
				CopyFixed(mFixedInsts[j],temp);
			}
		}
	}

	cout << "Leaving cPlotTask::OrderAllPred(). " << endl;
	return mFixedInsts.size();
}

//***************************************************************************
bool cPlotTask::CellCentriods()
{
	cout << "In cPlotTask::CellCentriods()	" << endl;
	cout << "Calculating cell centroids." << endl;

	pqxx::result r;
	string PreQuery = "update cell set centriod = ST_SetSRID(ST_MakePoint(";
	string query = "";
	string MidQuery = "),4326) where risector = ";
	char* temp;
	temp = new char[33];
	double lat, lon;

	unsigned i,j;
	unsigned CII = 0; //Current Installation Index
	unsigned NumInsts = mFixedInsts.size();
	cGeoP North;
	for (i=0; i<mRows; i++)
	{
		for (j=0; j<mCols; j++)
		{
			if (mPlot[i][j]>=0)
			{
				while ((mPlot[i][j]!=mFixedInsts[CII].sInstKey)&&(CII<NumInsts))
					CII++;
				if ((mPlot[i][j]!=mFixedInsts[CII].sInstKey)&&(CII>=NumInsts))
				{
					CII=0;
					while ((mPlot[i][j]!=mFixedInsts[CII].sInstKey)&&(CII<NumInsts))
						CII++;
				}
				if (mPlot[i][j]==mFixedInsts[CII].sInstKey)
				{
					mFixedInsts[CII].sCentroidX += j;
					mFixedInsts[CII].sCentroidY += i;
					mFixedInsts[CII].sPixelCount++;
				}
			}
		}
	}

	for (i=0; i<NumInsts; i++)
	{
		if (mFixedInsts[i].sPixelCount>0)
		{
			mFixedInsts[i].sCentroidX /=mFixedInsts[i].sPixelCount;
			mFixedInsts[i].sCentroidY /=mFixedInsts[i].sPixelCount; 
			mNorthWest.SetGeoType(DEG);
			mNorthWest.Get(lat,lon);
			lat-=mFixedInsts[i].sCentroidY*mNSres;
			lon+=mFixedInsts[i].sCentroidX*mEWres;
			mFixedInsts[i].sCentroid.Set(lat,lon,DEG);

			query = PreQuery;
			gcvt(lon,8,temp);
			query += temp;
			query += ",";
			gcvt(lat,8,temp);
			query += temp;
			query += MidQuery;
			gcvt(mFixedInsts[i].sInstKey,8,temp);
			query += temp;
			query += ";";	
			if(!gDb.PerformRawSql(query))
			{
				string err = "Error in Updating cell centroids. Query:";
				err+=query; 
				QRAP_ERROR(err.c_str());
				delete [] temp;
			}
		}
	}
	delete [] temp;
	return true;	
}

//***************************************************************************
double cPlotTask::TrafficDensCost()
{
	unsigned i, j;
	double TotalCost = 0.0;
	double SumPerRow = 0.0;

//	cout << "In cPlotTask::TrafficDensCost(): Before Calculation" << endl;
	for (i=0; i<mNumInstInTraffic; i++)
	{
		SumPerRow=0.0;
		for (j=1; j<=mNumClutter; j++)
		{
//			cout << "j=" << j << "	Digtheid=" << mVerkeerDigt[j] << endl;
//			cout << "i=" << i << "	j=" << j << "	Area=" << mClutterArea[i][j] << endl;
			SumPerRow += mVerkeerDigt[j]*mClutterArea[i][j];
		}
//		cout<< "i="<< i <<"	SumPerRow=" << SumPerRow 
//			<< "	mCellTraffic[i]="<< mCellTraffic[i] <<endl;
		TotalCost += (SumPerRow-mCellTraffic[i])*(SumPerRow-mCellTraffic[i]);
	}
//	cout << "In cPlotTask::TrafficDensCost(): After Calculation" << endl;
	TotalCost = sqrt(TotalCost);
	return TotalCost;
}

//***************************************************************************
bool cPlotTask::DetermineTrafficDist(bool Packet)
{
	unsigned i, j, k, n, m;
	string queryB;
	char* temp;
	temp = new char[33];
	cClutter ClutterUsed(mClutterClassGroup);
	mNumClutter = ClutterUsed.mNumber;
	cout << "In cPlottask::DetermineTrafficDist(). ClutterUsed.mNumber = " 
		<< ClutterUsed.mNumber << endl;	
	Float2DArray ClutterRaster;
	ClutterRaster = new_Float2DArray(2,2);
	cGeoP NW = mNorthWest;
	cGeoP SE = mSouthEast;
	double PlotRes=mPlotResolution;
	unsigned Rows = mRows;
	unsigned Cols = mCols;
	GeoType Type = NW.GetGeoType();
	cout << "In cPlottask::DetermineTrafficDist(). Getting Clutter ..."  << endl;
	mClutter.GetForDEM(NW, SE, PlotRes, Rows, Cols, ClutterRaster, Type, true);
	cout << "In cPlottask::DetermineTrafficDist() " << endl;
	cout << "PlotRes:	Voor:	" << mPlotResolution << "	Na:	" <<  PlotRes << endl;
	cout << "Rows:	Voor:	" << mRows<< "	Na:	" <<  Rows << endl;
	cout << "Cols:	Voor:	" << mCols<< "	Na:	" <<  Cols << endl;
	cout << "North West:" << endl;
	mNorthWest.Display();
	NW.Display();
	cout << "South East: " << endl;
	mSouthEast.Display();
	SE.Display();

	cout << "Number of installations in plot:	" << mFixedInsts.size() << endl;
	cout << "Deleting installations that will not be included in the traffic calculations." << endl;
	
	string query = "SELECT distinct radioinstallation_view.id AS radinst"; 
	query += " from radioinstallation_view cross join site_view_only cross join customareafilter";
	query += " WHERE (radioinstallation_view.siteid = site_view_only.id)";
	query += " and customareafilter.areaname ='TrafficArea' ";
	query += "and ST_Within(location,the_geom)";

	pqxx::result QResult;
	string errormessage="";
	if (!gDb.PerformRawSql(query))
	{
		errormessage =" In cPlottask::DetermineTrafficDist(). ";
		errormessage += "Error in query to get sites included in TrafficArea";
		cout << errormessage << endl;
		QRAP_ERROR(errormessage);
	}
	else
	{
		gDb.GetLastResult(QResult);
		if (QResult.size()>0)
		{
			unsigned ListSize=QResult.size();
			unsigned * InstList;
			InstList = new unsigned[ListSize];
 			for (i=0; i<ListSize; i++)
				InstList[i]=atoi(QResult[i]["radinst"].c_str());
			bool found = false;
			for (j=0; j<mFixedInsts.size(); j++)
			{
				found = false;
				for (i=0; (i<ListSize)&&(!found); i++)
					found = (mFixedInsts[j].sInstKey==InstList[i]);
				if (!found)
				{
					mFixedInsts.erase(mFixedInsts.begin()+j);
					j--;
				}
				else if ((0.0==mFixedInsts[j].sPStraffic)&&(0.0==mFixedInsts[j].sCStraffic))
				{
					mFixedInsts.erase(mFixedInsts.begin()+j);
					j--;
				}
					
			}
		}
		else
		{
			errormessage = "In cPlottask::DetermineTrafficDist(). ";
			errormessage += "Empty query to get sites included in TrafficArea\n";
			errormessage += "Is there a 'TrafficArea' polygon defined in the 'customareafilter' table? ";
			errormessage += "Query: ";
			errormessage += query;
			cout << errormessage;
			QRAP_ERROR(errormessage);
			return false;
		} 
	}

	mNumInstInTraffic = mFixedInsts.size();

	cout << "In cPlottask::DetermineTrafficDist(). mNumInstInTraffic = " << mNumInstInTraffic << endl;	
	for (n=0; n<mNumInstInTraffic; n++)
		cout << mFixedInsts[n].sInstKey << "	";
	cout << endl;

	mCellTraffic = new double[mNumInstInTraffic];

	if (Packet)
	{
		for (n=0; n<mNumInstInTraffic; n++)
			mCellTraffic[n] = mFixedInsts[n].sPStraffic; 
	}
	else
	{
		for (n=0; n<mNumInstInTraffic; n++)
			mCellTraffic[n] = mFixedInsts[n].sCStraffic; 
	}

	mClutterArea = new_Float2DArray(mNumInstInTraffic,ClutterUsed.mNumber+1); 
	for (i=0; i<mNumInstInTraffic; i++)
		for (j=0; j<=ClutterUsed.mNumber; j++)
			mClutterArea[i][j] = 0.0;

	// determine area matrix
	cout << "In cPlottask::DetermineTrafficDist(). Determining area Matrix " << endl; 
	unsigned CurrentRadInstID = 0;
	for (i=0; i<mRows; i++)
	{
		for (j=0; j<mCols; j++)
		{
			if (mPlot[i][j] != mFixedInsts[CurrentRadInstID].sInstKey)
			{
				CurrentRadInstID=0;
				while ((mFixedInsts[CurrentRadInstID].sInstKey!=mPlot[i][j])
						&&(CurrentRadInstID<mNumInstInTraffic-1))
					CurrentRadInstID++;
			}
//			cout << "CurrentRadInstID = " << CurrentRadInstID << "	Clutter = " << ClutterRaster[i][j] << endl;
			if ((mPlot[i][j] == mFixedInsts[CurrentRadInstID].sInstKey)
				&&((unsigned)ClutterRaster[i][j]<=ClutterUsed.mNumber))
				mClutterArea[CurrentRadInstID][(unsigned)ClutterRaster[i][j]] += PlotRes*PlotRes/1000/1000;
//			else cout << i << "," << j << "," << mPlot[i][j] << "," << ClutterRaster[i][j] << "	";
		}
	}
	cout << endl;

	//Determine starting values for trafficdensities
	
	double SumOfTraffic = 0.0;
	cout << "In cPlottask::DetermineTrafficDist(). Preparing starting Traffic Densities " << endl; 
	for (n=0; n<mNumInstInTraffic; n++)
	{
		cout << "Inst: " << mFixedInsts[n].sInstKey << "	Traffic=" << mCellTraffic[n] << endl;
		SumOfTraffic += mCellTraffic[n]; 
	}

	double sumOfArea = 0.0;
	double * AreaSumPerClutter;
	AreaSumPerClutter = new double[ClutterUsed.mNumber+1];
	for (j=0; j<=ClutterUsed.mNumber; j++)
		AreaSumPerClutter[j]=0.0;
	for (j=1; j<=ClutterUsed.mNumber; j++)
	{
		for (n=0; n<mNumInstInTraffic; n++)
		{
			AreaSumPerClutter[j]+=mClutterArea[n][j];
			sumOfArea+=mClutterArea[n][j];
		}
		cout << "Clutter=" << j << "	Area: " << AreaSumPerClutter[j] << endl;
	}

	mVerkeerDigt = new double[ClutterUsed.mNumber+1];
	double * VerkeerDigtVorige;
	VerkeerDigtVorige = new double[ClutterUsed.mNumber+1];	
	mVerkeerDigt[0] = 0.0; 	 
	VerkeerDigtVorige[0] = 0.0;
	for (j=1; j<=ClutterUsed.mNumber; j++)
	{
		if ((AreaSumPerClutter[j]>0.0)&&(j<=ClutterUsed.mNumber-3))
			mVerkeerDigt[j] = SumOfTraffic/sumOfArea;
		else
		{
			mVerkeerDigt[j]=0.0; // The cluttertype does not occur in the area.
			AreaSumPerClutter[j] = 0;
		}
		VerkeerDigtVorige[j] = mVerkeerDigt[j];
	}
	
	double SumPerRow=0, TotalCost=0;
	for (i=0; i<mNumInstInTraffic; i++)
	{
		SumPerRow=0.0;
		for (j=0; j<=mNumClutter; j++)
		{
			cout << "	" << i << "," << j << ",a:" << mClutterArea[i][j];
			SumPerRow += mVerkeerDigt[j]*mClutterArea[i][j];
		}
		cout << endl;
		cout<< "i="<< i <<"	SumPerRow=" << SumPerRow 
			<< "	mCellTraffic[i]="<< mCellTraffic[i] <<endl;
		TotalCost += (SumPerRow-mCellTraffic[i])*(SumPerRow-mCellTraffic[i]);
	}

	cout << "In cPlottask::DetermineTrafficDist(). Initialising derivatives" << endl; 
	double * dCdD; //partial derivative of Costfunction with respect to 
	dCdD = new double[ClutterUsed.mNumber+1];
	for (j=0; j<=ClutterUsed.mNumber; j++)
		dCdD[j] = 0.0;

	double OldCost, TestCost, Delta, SumOfdCdD, SizeOfdCdD, Cost;
	double StepSize = 0.01*SumOfTraffic/sumOfArea;
	bool Stop = false, Better=false;
	int IterLeft=5000;
	
	double MinTestCost, MinDelta;
	double MinCost = TrafficDensCost();
	unsigned MinIndex=0;
	cout << "In cPlottask::DetermineTrafficDist(). Starting optimisation loop. MinCost="<< MinCost << endl; 
	while ((!Stop)&&(IterLeft>0))
	{
		SumOfdCdD = 0.0;
		OldCost = TrafficDensCost();
		MinTestCost=OldCost;
		MinIndex=0;
		for (j=1; j<=ClutterUsed.mNumber; j++)
		{
			if (AreaSumPerClutter[j]>0)
			{
				Delta = max(0.01*mVerkeerDigt[j],0.0001*SumOfTraffic/sumOfArea);
				mVerkeerDigt[j]+=Delta;
				TestCost = TrafficDensCost();
				if (TestCost<MinTestCost)
				{
					MinIndex=j;
					MinTestCost=TestCost;
					MinDelta = Delta;
				}
				if (fabs(Delta)>0) dCdD[j] = (TestCost - OldCost)/Delta;
				else dCdD[j] = 0;
//				cout << j << "	TestCost=" << TestCost << "	OldCost=" << OldCost 
//					<< "	Delta= " << Delta << "	" << dCdD[j] << endl;
				if ((mVerkeerDigt[j]<0.0000001*SumOfTraffic/sumOfArea)&&(dCdD[j]>0))
					dCdD[j]=0.0;
				SumOfdCdD += dCdD[j]*dCdD[j];
				//return to original value
				mVerkeerDigt[j]-=Delta;
			}
		}

		SizeOfdCdD = sqrt(SumOfdCdD);
//		cout << "SumOfdCdD = " << SumOfdCdD << endl;
		StepSize=0.01*SumOfTraffic/sumOfArea;
		Better = false;
		while ((StepSize>0.001*SumOfTraffic/sumOfArea)&&(!Better)&&(SizeOfdCdD>0))
		{
			for (j=1; j<=ClutterUsed.mNumber; j++)
			{
				if (AreaSumPerClutter[j]>0)
				{
					VerkeerDigtVorige[j] = mVerkeerDigt[j];
					mVerkeerDigt[j]-=dCdD[j]*StepSize/SizeOfdCdD;
					if (mVerkeerDigt[j]<0.0)
						mVerkeerDigt[j]=0.0;
				}
			}
			Cost = TrafficDensCost();
			if (Cost<OldCost)
				Better=true;
			else 
			{
				for (j=1; j<=ClutterUsed.mNumber; j++)
				{
					mVerkeerDigt[j] = VerkeerDigtVorige[j];
				}
				StepSize*=0.7;
			}
		}
		IterLeft--;
		if (!Better)
		{
			if (MinIndex>0)
			{
				mVerkeerDigt[MinIndex]+=MinDelta;
				Cost = MinTestCost;
				cout << "MinIndex=" << MinIndex << "	" << MinDelta << "	" << Cost << endl;
			}
//			else Stop = true;	
		}
		if (Cost<=MinCost)
			MinCost = Cost;
		else Stop = true;
		if (IterLeft%10 ==0)
			cout << "#"<< IterLeft<<"	Cost=" << Cost << "	SumOfdCdD = " << SumOfdCdD << endl;
	}

	cout << "Results of Gradient Search" << endl;
	for (i=1; i<=ClutterUsed.mNumber; i++)
	{
		cout << "ClutterIndex=" << i 
			<< "	TrafficDensity = " << mVerkeerDigt[i] << endl;
	}

	for (i=0; i<mRows; i++)
	{
		for(j=0; j<mCols; j++)
		{
//			cout << "i=" << i << "	j=" << j << "	ClutterType=" << (unsigned)ClutterRaster[i][j] << endl;
			if ((unsigned)ClutterRaster[i][j] <= ClutterUsed.mNumber)
				mPlot[i][j] = mVerkeerDigt[(unsigned)ClutterRaster[i][j]];
			else mPlot[i][j] = 0.0;
		}
	}
	cout << "In cPlottask::DetermineTrafficDist(). Done with mPlot " << endl;
	delete_Float2DArray(ClutterRaster);
/*
	// Determine size of matrix ... i.e. How many Cluttertypes are represented
	cout << "In cPlottask::DetermineTrafficDist(). TotalsPerClutter " << endl; 
	double *TotalsPerClutter;
	TotalsPerClutter = new double[ClutterUsed.mNumber+1];
	for (j=0; j<=ClutterUsed.mNumber; j++)
	{
		TotalsPerClutter[j] = 0.0;
		for (i=0; i<mNumInstInTraffic; i++)
		{
//			cout << ClutterArea[i][j] << "	";
			TotalsPerClutter[j]+=mClutterArea[i][j];
		}
//		cout << endl;
//		cout << "ClutterIndex = " <<j <<"	" <<TotalsPerClutter[j] << endl;
	}
	
	unsigned ClutterCount = 0;
	for (j=0; j<=ClutterUsed.mNumber; j++)
	{
		if (TotalsPerClutter[j]>0) 
			ClutterCount++;
	}

	cout << "In cPlottask::DetermineTrafficDist(). ClutterCount = " << ClutterCount << endl; 
	cout << "In cPlottask::DetermineTrafficDist(). ClutterUsed.mNumber = " << ClutterUsed.mNumber << endl; 
	unsigned *ClutterIndex;
	ClutterIndex = new unsigned[ClutterCount];
	for (j=0; j<ClutterCount; j++)
	{
		ClutterIndex[j]=0;
	}

	unsigned Index = 0;
	for (j=1; j<=ClutterUsed.mNumber; j++)
	{
		cout << "j=" <<j <<"	" <<TotalsPerClutter[j] << endl;
		if ((TotalsPerClutter[j]>0)&&(Index<ClutterCount))
		{
			ClutterIndex[Index] = j;
			Index++;
		} 
	}
	cout << "Index = " << Index << endl;

	// Implementation of equating derivative to zero
	mTheMatrix.resize(ClutterCount,ClutterCount);
	mRight.resize(ClutterCount,1);

	for( m=0; m<ClutterCount; m++)
	{
		mRight(m,0)=0.0;
		for(k=0; k<ClutterCount; k++)
		{
			mTheMatrix(m,k) = 0.0;
		}
	}

	cout << "Completing matrixes" << endl;
	for (n=0; n<mNumInstInTraffic; n++)
	{	
		for( m=0; m<ClutterCount; m++)
		{
			mRight(m,0)+=mClutterArea[n][ClutterIndex[m]]*mCellTraffic[n];			
			for(k=0; k<ClutterCount; k++)
			{
				mTheMatrix(m,k) += mClutterArea[n][ClutterIndex[m]]
							*mClutterArea[n][ClutterIndex[k]];
			}
		}
	}
	
//	for( m=0; m<ClutterCount; m++)
//	{
//		cout << endl<< mRight(m,0) << endl;			
//		for(k=0; k<ClutterCount; k++)
//		{
//			cout << mTheMatrix(m,k) << "	";
//		}
//	}

	mTrafficDens = mTheMatrix.fullPivLu().solve(mRight);

	cout << "In cPlottask::DetermineTrafficDist() After finding zero of derivative solution" << endl;
	for (i=0;i<ClutterCount;i++)
	{
		cout << "ClutterIndex[i] = " << ClutterIndex[i] 
			<< "	mTrafficDens(i) = " << mTrafficDens(i) << endl;
		
	}

//	Determine if Function is convex.
	for( m=0; m<ClutterCount; m++)
	{
		mRight(m,0)=0.0;
		for(k=0; k<ClutterCount; k++)
		{
			mTheMatrix(m,k) = 0.0;
		}
	}
	for (n=0; n<mNumInstInTraffic; n++)
	{	
		for( m=0; m<ClutterCount; m++)
		{
			mRight(m,0)+=mClutterArea[n][ClutterIndex[m]]*mCellTraffic[n];			
			for(k=0; k<ClutterCount; k++)
			{
				mTheMatrix(m,k) += mClutterArea[n][ClutterIndex[m]]
							*mClutterArea[n][ClutterIndex[k]];
			}
		}
	}
	cout << "Determining Eigenvalues" << endl;
	SelfAdjointEigenSolver<MatrixXd> es(mTheMatrix,  EigenvaluesOnly);
	bool Convex=true;
	for (i=0; (i< es.eigenvalues().size())&&(Convex); i++)
		Convex= (es.eigenvalues()[i]>=0);

	if (Convex) cout << "In cPlottask::DetermineTrafficDist(). The function is CONVEX. " << endl;
	else cout << "In cPlottask::DetermineTrafficDist(). The function is NOT convex. " << endl;

	//create index of arrays where TrafficDensity is negative.
	bool Solution = true;
	std::vector <unsigned> ActiveSet;
	ActiveSet.clear();
	for (i=0;i<ClutterCount;i++)
	{
		if (mTrafficDens(i)<0)
		{
			ActiveSet.push_back(i);
			Solution = false;
		}
	}
	unsigned CurrentActiveSetSize = ActiveSet.size();
	unsigned OriginalSetSize = ActiveSet.size();
	unsigned *OldActiveSet;
	unsigned *OriginalSet;
	OldActiveSet = new unsigned[CurrentActiveSetSize];
	OriginalSet = new unsigned[OriginalSetSize];
	for (i=0;i<CurrentActiveSetSize;i++)
	{
		OldActiveSet[i]=ActiveSet[i];
		OriginalSet[i]=ActiveSet[i];
	}

	
	cout << " first make each induvidual constraint active " << endl;
	mTheMatrix.resize(ClutterCount+1,ClutterCount+1);
	mRight.resize(ClutterCount+1,1);
	for (j=0; (j<CurrentActiveSetSize)&&(!Solution); j++)
	{
		mRight(ClutterCount,0)=0.0;
		for( m=0; m<ClutterCount+1; m++)
		{
			mRight(m,0)=0.0;
			for(k=0; k<ClutterCount+1; k++)
			{
				mTheMatrix(m,k) = 0.0;
			}
		}

		for (n=0; n<mNumInstInTraffic; n++)
		{	
			for( m=0; m<ClutterCount; m++)
			{
				mRight(m,0)+=mClutterArea[n][ClutterIndex[m]]*mCellTraffic[n];			
				for(k=0; k<ClutterCount; k++)
				{
					mTheMatrix(m,k) += mClutterArea[n][ClutterIndex[m]]
								*mClutterArea[n][ClutterIndex[k]];
				}
			}
		}
 		mRight(ClutterCount,0)=0.0;
		for(k=0; k<ClutterCount; k++)
		{
			if (k==ActiveSet[j])
			{
				mTheMatrix(ClutterCount,k) = -1;
				mTheMatrix(k,ClutterCount) = -1;
			}
			else
			{
				mTheMatrix(ClutterCount,k) = 0;
				mTheMatrix(k,ClutterCount) = 0;
			}
		}
		mTheMatrix(ClutterCount,ClutterCount) = 0.0;	

//		for( m=0; m<ClutterCount+1; m++)
//		{
//			cout << endl<< mRight(m,0) << endl;			
//			for(k=0; k<ClutterCount+1; k++)
//			{
//				cout << mTheMatrix(m,k) << "	";
//			}
//		}
//		cout << endl;

		mTrafficDens = mTheMatrix.fullPivLu().solve(mRight);
		m=0;
		Solution = true;
		// Test for all traffic densities and langragian variables. 
		// If the traffic density is negative then this is not a solution
		// If one of the langragian variables is negative then it is not the optimal solution.  
		while ((m<ClutterCount+1)&&(Solution))
		{
			Solution = (mTrafficDens(m)>=0);
			m++;
		}
	}

	if (Solution) 
	{
		cout << "In cPlottask::DetermineTrafficDist(): Solution FOUND Clutter Index = " 
			<< ClutterIndex[j] << endl;	
		for (i=0;i<ClutterCount;i++)
		{
			cout << "ClutterIndex[i] = " << ClutterIndex[i] 
				<< "	mTrafficDens(i) = " << mTrafficDens(i) << endl;
		
		}
		delete [] TotalsPerClutter;
		delete [] ClutterIndex;
	}
	else cout << "In cPlottask::DetermineTrafficDist(): Solution NOT found" << endl; 


	mTheMatrix.resize(ClutterCount+CurrentActiveSetSize,ClutterCount+CurrentActiveSetSize);
	mRight.resize(ClutterCount+CurrentActiveSetSize,1);
	for( m=0; m<ClutterCount+CurrentActiveSetSize; m++)
	{
		mRight(m,0)=0.0;
		for(k=0; k<ClutterCount+CurrentActiveSetSize; k++)
		{
			mTheMatrix(m,k) = 0.0;
		}
	}

	for (n=0; n<mNumInstInTraffic; n++)
	{	
		for( m=0; m<ClutterCount; m++)
		{
			mRight(m,0)+=mClutterArea[n][ClutterIndex[m]]*mCellTraffic[n];			
			for(k=0; k<ClutterCount; k++)
			{
				mTheMatrix(m,k) += mClutterArea[n][ClutterIndex[m]]
							*mClutterArea[n][ClutterIndex[k]];
			}
		}
	}

	for (j=ClutterCount; j<ClutterCount+CurrentActiveSetSize; j++)
	{
		mRight(j,0)=0.0;
		for(k=0; k<ClutterCount; k++)
		{
			if (k==OldActiveSet[j-ClutterCount])
			{
				mTheMatrix(j,k) = -1;
				mTheMatrix(k,j) = -1;
			}
			else
			{
				mTheMatrix(j,k) = 0.0;
				mTheMatrix(k,j) = 0.0;
			}
		}
		for (k=ClutterCount; k<ClutterCount+CurrentActiveSetSize; k++)
		{
			mTheMatrix(j,k) = 0.0;
			mTheMatrix(k,j) = 0.0;
		}
	}

//	for( m=0; m<ClutterCount+CurrentActiveSetSize; m++)
//	{
//		cout << endl<< mRight(m,0) << endl;			
//		for(k=0; k<ClutterCount+CurrentActiveSetSize; k++)
//		{
//			cout << mTheMatrix(m,k) << "	";
//		}
//	}
//	cout << endl;

	mTrafficDens = mTheMatrix.fullPivLu().solve(mRight);
	cout << "Traffic density after all constraints was in active set" << endl;
	m=0;
	Solution = true;
	// Test for all traffic densities and langragian variables. 
	// If the traffic density is negative then this is not a solution
	// If one of the langragian variables is negative then it is not the optimal solution.  
	cout << "CurrentActiveSetSize = " << CurrentActiveSetSize << endl;
	m=0;
	while ((m<(ClutterCount+CurrentActiveSetSize))&&(Solution))
	{
		cout << "ClutterIndex=" << ClutterIndex[m] << "	Traffic Density = " << mTrafficDens(m) << endl;
		Solution = (mTrafficDens(m)>=0);
		m++;
	}

	unsigned MAXattempts = ClutterCount;
	unsigned numAttempts=0;
	while ((numAttempts<=MAXattempts)&&(!Solution))
	{
		ActiveSet.clear();
		for (j=0; j < OriginalSetSize; j++)
		{
			if (mTrafficDens(OriginalSet[j])<-0.00001)
			{
				ActiveSet.push_back(OriginalSet[j]);
			}
		}
		for (j=0; j < CurrentActiveSetSize; j++)
		{
			if ((0==mTrafficDens(OldActiveSet[j]))&&(mTrafficDens(j+ClutterCount)>=0))
				ActiveSet.push_back(OldActiveSet[j]);
		}
		if (CurrentActiveSetSize == ActiveSet.size())
			ActiveSet.erase(ActiveSet.begin() + numAttempts % CurrentActiveSetSize);
		CurrentActiveSetSize = ActiveSet.size();
		delete [] OldActiveSet;
		OldActiveSet = new unsigned[CurrentActiveSetSize];
		for (i=0;i<CurrentActiveSetSize;i++)
			OldActiveSet[i]=ActiveSet[i];
		
		mTheMatrix.resize(ClutterCount+CurrentActiveSetSize,ClutterCount+CurrentActiveSetSize);	
		mRight.resize(ClutterCount+CurrentActiveSetSize,1);
		for( m=0; m<ClutterCount+CurrentActiveSetSize; m++)
		{
			mRight(m,0)=0.0;
			for(k=0; k<ClutterCount+CurrentActiveSetSize; k++)
			{
				mTheMatrix(m,k) = 0.0;
			}
		}
		for (n=0; n<mNumInstInTraffic; n++)
		{	
			for( m=0; m<ClutterCount; m++)
			{
				mRight(m,0)+=mClutterArea[n][ClutterIndex[m]]*mCellTraffic[n];			
				for(k=0; k<ClutterCount; k++)
				{
					mTheMatrix(m,k) += mClutterArea[n][ClutterIndex[m]]
								*mClutterArea[n][ClutterIndex[k]];
				}
			}
		}
	
		for (j=0; (j<CurrentActiveSetSize); j++)
		{
			mRight(j+ClutterCount,0)=0.0;
			for(k=0; k<ClutterCount; k++)
			{
				if (k==OldActiveSet[j])
				{
					mTheMatrix(j+ClutterCount,k) = -1;
					mTheMatrix(k,j+ClutterCount) = -1;
				}
				else
				{
					mTheMatrix(j+ClutterCount,k) = 0;
					mTheMatrix(k,j+ClutterCount) = 0;
				}
			}
			for (k=ClutterCount; k<ClutterCount+CurrentActiveSetSize; k++)
			{
				mTheMatrix(j+ClutterCount,k) = 0;
				mTheMatrix(k,j+ClutterCount) = 0;
			}
		}

//		for( m=0; m<ClutterCount+CurrentActiveSetSize; m++)
//		{
//			cout << endl<< mRight(m,0) << endl;			
//			for(k=0; k<ClutterCount+CurrentActiveSetSize; k++)
//			{
//				cout << mTheMatrix(m,k) << "	";
//			}
//		}
//		cout << endl;
	
		mTrafficDens = mTheMatrix.fullPivLu().solve(mRight);
		m=0;
		Solution = true;
		// Test for all traffic densities and langragian variables. 
		// If the traffic density is negative then this is not a solution
		// If one of the langragian variables is negative then it is not the optimal solution.  
		cout << "CurrentActiveSetSize = " << CurrentActiveSetSize << endl;
		while ((m<ClutterCount+CurrentActiveSetSize)&&(Solution))
		{
			cout << "ClutterIndex=" << ClutterIndex[m] << "	Traffic Density = " << mTrafficDens(m) << endl;
			Solution = (mTrafficDens(m)>=-0.000001);
			m++;
		}
		while ((m<CurrentActiveSetSize)&&(Solution))
		{
			cout << "ClutterIndex=" << ClutterIndex[ActiveSet[m]] 
				<< "	Traffic Density = " << mTrafficDens(ActiveSet[m]) 
				<< "	lamdda= "<< mTrafficDens(m+ClutterCount) <<	endl;
			Solution = (0==mTrafficDens(m+ClutterCount)*mTrafficDens(ActiveSet[m]));
			m++;
		}
		numAttempts++;
	}
	delete [] OldActiveSet;
	

	cout << "In cPlottask::DetermineTrafficDist():  Saving estimations" << endl;
	queryB = "INSERT into TrafficDensity ";
	queryB += "(lastmodified, traffictype, technology, cluttertype, value) values ";
	queryB += "(now(), ";
	if (Packet) queryB += "'Packet Switched', ";
	else queryB += "'Circuit Switched', ";
	gcvt(mFixedInsts[0].sTechKey,8,temp);
	queryB += temp;
	queryB += ", ";

	for (m=0; m<ClutterCount; m++)
	{	
		query = queryB;
		gcvt(ClutterIndex[m],8,temp);
		query += temp;
		query += ", ";
		gcvt(mTrafficDens(m),8,temp);
		query += temp;
		query += ");";
	}
	delete [] TotalsPerClutter;
	delete [] ClutterIndex;	


	// Implementation of 'straight' matrix inversion

	if (mNumInstInTraffic<ClutterCount)
	{
		cout << "Too few cells are included. Retry with more cells." << endl;
		delete [] ClutterIndex;
		delete [] TotalsPerClutter;
		delete_Float2DArray(mClutterArea);
		return false;
	}

	//Determine total area per Installation
	double *TotalsPerInst;
	TotalsPerInst = new double[mNumInstInTraffic];
	for (i=0; i<mNumInstInTraffic; i++)
	{
		TotalsPerInst[i] = 0.0;
		for (j=0; j<=ClutterUsed.mNumber; j++)
		{
			TotalsPerInst[i]+=mClutterArea[i][j];
//			cout << mClutterArea[i][j] << "	";
		}
//		cout << endl;
	}

	mTheMatrix.resize(ClutterCount,ClutterCount);
	mRight.resize(ClutterCount,1);

	for( m=0; m<ClutterCount; m++)
	{
		mRight(m,0)=0.0;
		for(k=0; k<ClutterCount; k++)
		{
			mTheMatrix(m,k) = 0.0;
		}
	}

	// Fill in square matrix
	unsigned *RadInstOrder;
	RadInstOrder = new unsigned[mNumInstInTraffic];
	for(i=0; i<mNumInstInTraffic; i++)
	{
		RadInstOrder[i] = i;
	}
	
	
	unsigned swap;
	for (i=0;i<mNumInstInTraffic;i++)
	{
		for (j=i+1; j<mNumInstInTraffic; j++)
		{
			if (TotalsPerInst[RadInstOrder[j]]>TotalsPerInst[RadInstOrder[i]])
			{
				swap = RadInstOrder[i];
				RadInstOrder[i] = RadInstOrder[j];
				RadInstOrder[j] = swap; 
			}
		}
	}

	delete [] TotalsPerInst;

	cout << "In cPlottask::DetermineTrafficDist() before Completing matrixes" << endl;

	for(i=0; i<ClutterCount; i++)
	{
//		cout << "i=" << i << "	RadInstOrder[i] = " << RadInstOrder[i] << endl;
		mRight(i,0) = mCellTraffic[RadInstOrder[i]];
		for(j=0; j<ClutterCount; j++)
		{
//			cout << "j=" << j << ": " << ClutterIndex[j] << "	";
			mTheMatrix(i,j) = mClutterArea[RadInstOrder[i]][ClutterIndex[j]];
		}
//		cout << endl;
	}

	unsigned CountWrap = ceil(((double)mNumInstInTraffic)/((double)ClutterCount));
	for (k=1;k<=CountWrap; k++)
	{
		for (i=ClutterCount*k+1;i<min(mNumInstInTraffic,ClutterCount*(k+1));i++)
		{
			mRight(ClutterCount-(i-ClutterCount*k),0) += mCellTraffic[RadInstOrder[i]];
			for(j=0; j<ClutterCount; j++)
				mTheMatrix(ClutterCount-(i-ClutterCount*k),j) += mClutterArea[RadInstOrder[i]][ClutterIndex[j]];
		}
 	}

	delete [] RadInstOrder;

	cout << "In cPlottask::DetermineTrafficDist() before Solving 'straight' matrix inversion " << endl;

	mTrafficDens = mTheMatrix.fullPivLu().solve(mRight);
	cout << "In cPlottask::DetermineTrafficDist() After 'straight' matrix invertion" << endl;
	for (i=0;i<ClutterCount;i++)
	{
		cout << "ClutterIndex[i] = " << ClutterIndex[i] 
			<< "	mTrafficDens(i) = " << mTrafficDens(i) << endl;
		
	}
*/

	cout << "In cPlottask::DetermineTrafficDist():  Saving estimations" << endl;

	queryB = "INSERT into TrafficDensity ";
	queryB += "(lastmodified, traffictype, technology, cluttertype, value) values ";
	queryB += "(now(), ";
	if (Packet) queryB += "'Packet Switched', ";
	else queryB += "'Circuit Switched', ";
	gcvt(mFixedInsts[0].sTechKey,8,temp);
	queryB += temp;
	queryB += ", ";

	for (m=1; m<=mNumClutter; m++)
	{	
		query = queryB;
		gcvt(m,8,temp);
		query += temp;
		query += ", ";
		gcvt(mVerkeerDigt[m],8,temp);
		query += temp;
		query += ");";
	}

	cout << "In cPlottask::DetermineTrafficDist() Leaving" << endl;
	delete [] temp;
	delete [] mCellTraffic;
	delete [] mVerkeerDigt;
	delete_Float2DArray(mClutterArea);
	return true;
}

//***************************************************************************
bool cPlotTask::GetDBinfo()
{
	pqxx::result r;
	string query;
	char* temp;
	temp = new char[33];
	unsigned i,j;
	double dloffset, upoffset, chansep; 
	query = "SELECT eirp,txpower,txlosses,rxlosses,rxsensitivity,antpatternkey,mobileheight,btlfreq,maxpathloss ";
	query += "FROM mobile CROSS JOIN technology ";
	query += "WHERE mobile.techkey=technology.id AND mobile.id=";
	gcvt(mMobile.sInstKey,8,temp);
	query += temp;
	query += ";";
	
	if(!gDb.PerformRawSql(query))
	{
		string err = "Error in Database Select on tables mobile and technology. Query:";
		err+=query; 
		QRAP_ERROR(err.c_str());
		return false;
	}
	else
	{
		gDb.GetLastResult(r);
		
		if(r.size()!=0)
		{
			mMobile.sEIRP = atof(r[0]["eirp"].c_str());
			mMobile.sTxPower = atof(r[0]["txpower"].c_str());
			mMobile.sTxSysLoss = atof(r[0]["txlosses"].c_str());
			mMobile.sRxSysLoss = atof(r[0]["rxlosses"].c_str());
			mMobile.sRxSens = atof(r[0]["rxsensitivity"].c_str());
			mMobile.sPatternKey = atoi(r[0]["antpatternkey"].c_str());
			mMobile.sMobileHeight = atof(r[0]["mobileheight"].c_str());
			mMobile.sFrequency = atof(r[0]["btlfreq"].c_str());
			mMaxPathLoss = atof(r[0]["maxpathloss"].c_str());
		} // if r.size()
	} // else !gDb->PerformRawSq
	
	unsigned n=0;
	double aveUpFreq=0.0;
	string PointString;
	double longitude, latitude; 
	unsigned spacePos;
	double channel;
	// Populate the rest of the fixed installation information from the database.
	for(i=0 ; i<mFixedInsts.size() ; i++)
	{
		gcvt(mFixedInsts[i].sInstKey,8,temp);
		query = "SELECT siteid, ST_AsText(location) as location, eirp, max(cell.txpower) as txpower,";
		query += "txlosses, txantpatternkey,txbearing,txmechtilt, txantennaheight,";
		query += "rxantpatternkey,rxbearing,rxmechtilt,rxlosses,rxsensitivity,rxantennaheight, ";
		query += "btlfreq, spacing, bandwidth, downlink, uplink, ";
		query += "sum(cstraffic) as cstraffic, sum(pstraffic) as pstraffic, techkey ";
		query += "FROM radioinstallation_view LEFT OUTER JOIN cell on (radioinstallation_view.id=risector) ";
		query += "CROSS JOIN technology CROSS JOIN site_view ";
		query += "WHERE techkey=technology.id and siteid=site_view.id ";
		query += " and radioinstallation_view.id ="; 
		query += temp;
		query += " group by siteid, location, eirp, txlosses, rxlosses,";
		query += "txantpatternkey, txbearing, txmechtilt, ";
		query += "rxantpatternkey, rxbearing, rxmechtilt, txantennaheight, rxantennaheight, ";
		query += "techkey, btlfreq, spacing, bandwidth, downlink, uplink, rxsensitivity;";
				
		// Perform a Raw SQL query
		if(!gDb.PerformRawSql(query))
		{
			string err = "Error in Database Select on tables radioinstallation and technology failed. Query:";
			err+=query; 
			QRAP_ERROR(err.c_str());
			delete [] temp;
			return false;
		} // if
		else
		{
			gDb.GetLastResult(r);
			
			int size = r.size();
			
			if(size!=0)
			{
				mFixedInsts[i].sSiteID = atoi(r[0]["siteid"].c_str());
				PointString = r[0]["location"].c_str();
				spacePos = PointString.find_first_of(' ');
				longitude = atof((PointString.substr(6,spacePos).c_str())); 
				latitude = atof((PointString.substr(spacePos,PointString.length()-1)).c_str());
				mFixedInsts[i].sSitePos.Set(latitude,longitude,DEG);
				mFixedInsts[i].sEIRP = atof(r[0]["eirp"].c_str());
                mFixedInsts[i].sTxPower = atof(r[0]["txpower"].c_str()); //! 0?
				mFixedInsts[i].sTxSysLoss = atof(r[0]["txlosses"].c_str());
				mFixedInsts[i].sRxSysLoss = atof(r[0]["rxlosses"].c_str());
				mFixedInsts[i].sRxSens = atof(r[0]["rxsensitivity"].c_str());
                mFixedInsts[i].sTxPatternKey = atoi(r[0]["txantpatternkey"].c_str()); //! key 4
				mFixedInsts[i].sTxAzimuth = atof(r[0]["txbearing"].c_str());
				mFixedInsts[i].sTxMechTilt = atof(r[0]["txmechtilt"].c_str());
				mFixedInsts[i].sRxPatternKey = atoi(r[0]["rxantpatternkey"].c_str());
				mFixedInsts[i].sRxAzimuth = atof(r[0]["rxbearing"].c_str());
				mFixedInsts[i].sRxMechTilt = atof(r[0]["rxmechtilt"].c_str());
				mFixedInsts[i].sTxHeight = atof(r[0]["txantennaheight"].c_str());
				mFixedInsts[i].sRxHeight = atof(r[0]["rxantennaheight"].c_str());
				mFixedInsts[i].sFrequency = atof(r[0]["btlfreq"].c_str());
				mFixedInsts[i].sBandWidth = atof(r[0]["bandwidth"].c_str());
				mFixedInsts[i].sCStraffic = atof(r[0]["cstraffic"].c_str());
				mFixedInsts[i].sPStraffic = atof(r[0]["pstraffic"].c_str());
				mFixedInsts[i].sTechKey = atof(r[0]["techkey"].c_str());
                dloffset = atof(r[0]["downlink"].c_str()); //! 1890
                upoffset = atof(r[0]["uplink"].c_str());    //! 1880
                chansep = atof(r[0]["spacing"].c_str());    //! 100
				
				query = "SELECT channel ";
				query += "FROM cell CROSS JOIN frequencyallocationlist ";
				query += "WHERE frequencyallocationlist.ci=cell.id and cell.risector ="; 
				gcvt(mFixedInsts[i].sInstKey,8,temp);
				query += temp;
				query += " order by carrier;";
								
				// Perform a Raw SQL query
				if(!gDb.PerformRawSql(query))
				{
					string err = "Error in Database Select on tables radioinstallation and technology failed. Query:";
					err+=query; 
					QRAP_ERROR(err.c_str());
				} // if
				else
				{
                    gDb.GetLastResult(r); //! size = 0, thus not entering the following if size>0
					unsigned size = r.size();
					if (size>0)
					{
						channel = atof(r[0]["channel"].c_str());
						mMobile.sFrequency = channel*chansep/1000.0+upoffset;
						mFixedInsts[i].sFrequency = channel*chansep/1000.0+dloffset;
						n++;
						aveUpFreq +=channel*chansep/1000.0+upoffset;
						mFixedInsts[i].sFreqList.push_back(channel*chansep/1000.0+dloffset);
						for (j=1; j<size; j++)
							mFixedInsts[i].sFreqList.push_back(atof(r[j]["channel"].c_str())*chansep/1000.0+dloffset);
					}
				}
			}
			else	//! if size==0 
			{
				mFixedInsts.erase(mFixedInsts.begin()+i);
				string err = "Warning, radio installation ";
				gcvt(mFixedInsts[i].sInstKey,8,temp); 
				err += temp;
				err += " does not exist in the database. ";
				err += "No prediction wil be done for it.";
				QRAP_WARN(err.c_str());
			} // else
		} // else
	} // for
	if (n>0) mMobile.sFrequency = aveUpFreq/n;
    cout<<"perform sql for mMobile,  n: "<<n<<endl;
//    getchar();
	delete [] temp;
	return true;
}


//***************************************************************************
bool cPlotTask::GetDBIntInfo()
{
	pqxx::result r;
	string query;
	char *temp;
	temp = new char[33];
	unsigned i;

	string radInstList= "";
	string PointString;
	gcvt(mFixedInsts[0].sInstKey,8,temp);
	radInstList += temp;
	// Populate the rest of the fixed installation information from the database.
	for(i=1 ; i<mFixedInsts.size() ; i++)
	{
		radInstList += ",";
		gcvt(mFixedInsts[i].sInstKey,8,temp);
		radInstList += temp;
	}
	cout << radInstList << endl;
	gcvt(mMaxRange,8,temp);
	query = "select distinct interRadInst.id as id from "; 
	query += "radioinstallation_view RadInst ";
	query += "CROSS JOIN radioinstallation_view as interRadInst "; 
	query += "CROSS JOIN site CROSS JOIN site as interSite "; 
	query += "CROSS JOIN cell CROSS JOIN cell as interCell ";
	query += "CROSS JOIN frequencyallocationlist as Freq "; 
	query += "CROSS JOIN frequencyallocationlist as interFreq ";
	query += "WHERE RadInst.siteid=site.id ";
	query += "AND interRadInst.siteid=interSite.id ";
	query += "AND cell.risector = RadInst.id ";
	query += "AND interCell.risector = interRadInst.id ";	
	query += "AND Freq.ci = cell.id ";
	query += "AND interFreq.ci = interCell.id ";
	query += "AND Freq.channel = interFreq.channel ";
	query += "AND Freq.id <> interFreq.id ";
	query += "AND ST_Distance(site.location,interSite.location) <";
	query += temp;
	query += "AND RadInst.id in (";
	query += radInstList;
	query += ");";
						
	// Perform a Raw SQL query
	if(!gDb.PerformRawSql(query))
	{
		string err = "Error in Database Select to get Interfering RadioInst. Query:";
		err+=query; 
		QRAP_ERROR(err.c_str());
		delete [] temp;
		return false;
	} // if
	else
	{
		gDb.GetLastResult(r);
		
		unsigned size = r.size();
		tFixed tempInst;
		if (size>0)
		{
			mFixedInsts.clear();
			cout << "NumInt: " << endl;
			for (i=0; i<size; i++)
			{
				tFixed tempInst;
				tempInst.sInstKey = atoi(r[i]["id"].c_str());
				tempInst.sRange = mMaxRange;
				tempInst.sCentroidX = 0;
				tempInst.sCentroidY = 0;
				tempInst.sPixelCount = 0;
				mFixedInsts.push_back(tempInst);
			}
		}
		GetDBinfo();
	}
	delete [] temp;
	return true;
}

//***************************************************************************
bool cPlotTask::WriteOutput(GeoType OutputProj)
{
	string err = "Writing output for requested Prediction.";
	cout << err << endl;
//	QRAP_INFO(err.c_str());
	cRaster Output;
	bool returnValue;
	
	mNorthWest.SetGeoType(DEG);
	mSouthEast.SetGeoType(DEG);

	returnValue = Output.WriteFile(mPlot, mNorthWest, mSouthEast, mRows, mCols,
							mNSres,mEWres,mDir,mOutputFile,HFA,DEG,	OutputProj,mCentMer);
	err = "After WriteOutput in cPlotTask.";
	cout << err << endl;
	return returnValue;
//	QRAP_INFO(err.c_str());
}
