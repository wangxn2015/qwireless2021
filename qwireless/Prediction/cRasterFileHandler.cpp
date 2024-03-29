 /*
 *    QRAP Project
 *
 *    Version        : 0.1
 *    Date            : 2008/04/01
 *    License        : GNU GPLv3
 *    File        		  : cRasterFileHandler.cpp
 *    Copyright      : (c) University of Pretoria
 *    Author          : Magdaleen Ballot (magdaleen.ballot@up.ac.za)
 *    Description   : Handler of one or many raster files ... gets the info out
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

#include "cRasterFileHandler.h"
#include "cRaster.h"

using namespace Qrap;

//*********************************************************************
cRasterFileHandler::cRasterFileHandler()
{
	mSampleMethod=1;
	mPreferedSetNW.Set(-90,180);
	mPreferedSetSE.Set(90,-180);
	mAvailableSetNW.Set(-90,180);
	mAvailableSetSE.Set(90,-180);
	mType = "NULL";
	mJumped =false;
	mCurrent = 0;
}

//*********************************************************************
void cRasterFileHandler::DirectChangeSetToUse(int OriginalSet)
{
	mFileSetOrder.clear();
	mFileSetOrder.push_back(OriginalSet);
	mCurrent=0;
}

//*********************************************************************
cRasterFileHandler::cRasterFileHandler(short int Source)
{
	mSampleMethod=1;
	mType = "NULL";
	SetRasterFileRules(Source);
	mPreferedSetNW.Set(-90,180);
	mPreferedSetSE.Set(90,-180);
	mAvailableSetNW.Set(-90,180);
	mAvailableSetSE.Set(90,-180);
}

//*********************************************************************
cRasterFileHandler::~cRasterFileHandler()
{
	for (unsigned i=0; i<mCurrentRasters.size(); i++)
		delete mCurrentRasters[i];
	mCurrentRasters.clear();
	mFileSetOrder.clear();
}

//*********************************************************************
bool cRasterFileHandler::SetRasterFileRules(short int RuleKey)
{
	mJumped = false;
	pqxx::result r;
	string QueryResult;
	int temp;
	unsigned i,NumBytes;
	char *RuleStr;
	RuleStr = new char[33];
	gcvt(RuleKey,8,RuleStr);
	
	string CmdStr("SELECT orderarray, type FROM filesetsused WHERE id=");
	CmdStr += RuleStr;  
	CmdStr +=";";
	
//	cout << CmdStr << endl;
	if(!gDb.PerformRawSql(CmdStr))
	{
		string err = "Getting the fileset to use failed. Query: ";
		err += CmdStr;
		QRAP_ERROR(err.c_str());
		return false;
	}
	else
	{
		gDb.GetLastResult(r);
		cout << "Number of filesets in fileset order: " << r.size() << endl;
		if(r.size()>0)
		{
 			// \TODO: I am sure there are more elegant ways, but for now it will do
			QueryResult=r[0]["orderarray"].c_str();
			NumBytes = QueryResult.length();
			i=1;
   			while ((i<NumBytes)&&(QueryResult[i]!='}'))
   			{
   				temp = 0;
   				while ((QueryResult[i]!=',')&&(QueryResult[i]!='}'))
			    	{
				 	temp =  temp*10 + (QueryResult[i]-'0');
   					i++;
   				}
			    	i++;
			    	mFileSetOrder.push_back(temp);
				cout << " cRasterFileHandler::SetRasterFileRules. RuleKey = " << 
					RuleKey << "	FileSet = " << temp << endl;
				
				
			}// end while i<NumBytes
   			mType = r[0]["type"].c_str();
   			if (mType=="DEM") mSampleMethod = 2;
   			else mSampleMethod = 1;
			cout << mType << endl;
		}//end if r.size
		else
		{
			string err = "There was no FileSetOrder. Query: ";
			err += CmdStr;
			QRAP_ERROR(err.c_str());
			return false;	
		}//end else r.size
	}//end else !gDb->PerformRawSql
	mCurrent=0;
	delete [] RuleStr;
	return true;
} //end SetRasterFileRules.


//************************************************************************
unsigned cRasterFileHandler::GetClutterClassGroup()
{
	int ClassGroup;
	char *temp;
	temp = new char[33];	
	pqxx::result CC;
	gcvt(mFileSetOrder[0],9,temp);
	string query = "SELECT classgroup from filesets WHERE ID="; 
	query += temp;	
	query += ";";

	cout << query << endl; 
	if(!gDb.PerformRawSql(query))
	{
		string err = "Getting the Clutter Classification Group to use failed. Query: ";
		err += query;
		QRAP_WARN(err.c_str());
		ClassGroup = 0;
	}
	else
	{
		gDb.GetLastResult(CC);
		if(CC.size()!=0)
		{
			ClassGroup = atoi(CC[0]["classgroup"].c_str());
			cout << "cRasterFileHandler::GetClutterClassGroup() ClassGroup = " << ClassGroup << endl;
		}
		else
		{
			string err = "The query for the Clutter Classification Group is empty. Query: ";
			err += query;
			QRAP_WARN(err.c_str());
			ClassGroup = -100;	
		}//end else CC.size
	}
	mCurrent=0;
	delete [] temp;	
	return ClassGroup;
};

//**********************************************************************
bool cRasterFileHandler::GetForLink(cGeoP TxLoc, cGeoP RxLoc, double DistRes, cProfile &OutProfile)
{
	double Distance, Bearing;
	unsigned NumPoints;
	bool IsInSet=false;
	bool IsInThis=false;
	bool AllFound=true;
	unsigned i,j,k=0;
	float *profile;
//	TxLoc.SetGeoType(DEG);
//	RxLoc.SetGeoType(DEG);
	TxLoc.SetGeoType(WGS84GC);
	RxLoc.SetGeoType(WGS84GC);
	Distance = TxLoc.Distance(RxLoc);
	Bearing = TxLoc.Bearing(RxLoc);
	NumPoints = (int)ceil(Distance/DistRes);
	DistRes = Distance/(double)NumPoints;
	NumPoints ++;
	profile = new float[NumPoints];
	cGeoP point;
	point.SetGeoType(WGS84GC);
	string LoadedRastersList="'";

//	cout << "In cRasterFileHandler::GetForLink(...). before loading file for TxLoc" << endl;


	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInThis = ((mCurrentRasters[i]->IsIn(TxLoc))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		IsInSet = IsInSet || IsInThis ;
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(TxLoc,LoadedRastersList);
		cout << "Adding for TxLoc: " << endl;
		TxLoc.Display();
		mCurrent=0;
	}

//	cout << "In cRasterFileHandler::GetForLink(...). before loading file for RxLoc" << endl;

	LoadedRastersList="'";
	IsInSet=false;
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInThis = ((mCurrentRasters[i]->IsIn(RxLoc))
				&&(0==mCurrentRasters[i]->mFileSetLevel));		
		IsInSet = IsInSet || IsInThis;
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(RxLoc,LoadedRastersList);
		cout << "Adding for RxLoc: " << endl;
		RxLoc.Display();
	}
	
//	cout << "In cRasterFileHandler::GetForLink(...). After loading file for RxLoc" << endl;


	if (mCurrentRasters.size()==0)
	{
		string err = "cRasterFileHandler::GetForLink;  No raster files could be found. Confirm that File-Set Order exist. ";
		cout << err << endl;
		QRAP_ERROR(err.c_str());
		return false;
	}
	
	IsInSet = (mCurrentRasters[mCurrent]->IsIn(TxLoc))&&(0==mCurrentRasters[k]->mFileSetLevel);
	if (!IsInSet)
	{
		mCurrent=0;
		LoadedRastersList="'";
		k=mCurrentRasters.size(); 
		while ((k>0)&&(!IsInSet))
		{
			k--;
			LoadedRastersList+=mCurrentRasters[k]->mFilename;
			LoadedRastersList+="','";
			IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(TxLoc))&&(0==mCurrentRasters[k]->mFileSetLevel));

		}
		if (!IsInSet)
		{
			LoadedRastersList+="'";
			IsInSet = AddRaster(TxLoc,LoadedRastersList);
			cout << "Adding for TxLoc2: " << endl;
			TxLoc.Display();
			mCurrent=0;
			k=mCurrentRasters.size();
			IsInSet = false;
			while ((k>mCurrentRasters.size())&&(!IsInSet))
			{
				k--;
				IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(TxLoc));
			}
		}

		if (IsInSet) mCurrent = k;
	}
	

	if (IsInSet)
	{		
		profile[0] = mCurrentRasters[mCurrent]->GetValue(TxLoc,mSampleMethod);
//		if (mJumped) 
//		{
//			cout <<"TxLoc1:" << mCurrentRasters[mCurrent]->mFilename<< endl;
//			TxLoc.Display();
//		}
		mCurrentRasters[mCurrent]->mUsed = true;
	}
	else
	{
		mCurrent = 0;
		profile[0]=OUTOFRASTER;
		AllFound = false;
	}

	bool Preferred=(0==mCurrentRasters[mCurrent]->mFileSetLevel);
	bool ToSwitch=false;
	bool Available = true;

	for(j=1; j<NumPoints; j++)
	{
		point.FromHere(TxLoc,j*DistRes,Bearing);
		if (!Preferred)
		{
			ToSwitch=point.Between(mPreferedSetNW,mPreferedSetSE);
			Available=point.Between(mAvailableSetNW,mAvailableSetSE);
		}
		else ToSwitch=false;
		
		profile[j] = mCurrentRasters[mCurrent]->GetValue(point,mSampleMethod);
		if ((profile[j] <-440.0)||(profile[j] >8880)||(profile[j]==OUTOFRASTER)||(0==profile[j]))
		{
			Available=point.Between(mAvailableSetNW,mAvailableSetSE);
		}

		//Took the below out and inserted the above 2018-05-20 		
/*		if (Available)
		{
			if (mCurrentRasters[Current]->IsIn(point))
				profile[j] = mCurrentRasters[Current]->GetValue(point,mSampleMethod);
			
		}
		else profile[j]=OUTOFRASTER;
*/
//		cout << j << "	" << profile[j] << endl;

		if (((profile[j] <-440.0)||(profile[j] >8880)||(profile[j]==OUTOFRASTER)||(ToSwitch))&&(Available))
		// to allow for the Dead Sea shore and Mount Everest
		{
			LoadedRastersList="'";
			k = mCurrentRasters.size();
			IsInSet= false;
			while ((k>0)&&(!IsInSet))
			{
				k--;
				LoadedRastersList+=mCurrentRasters[k]->mFilename;
				LoadedRastersList+="','";
				IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(point))&&
						(0==mCurrentRasters[k]->mFileSetLevel));
			}
			if ((!IsInSet)&&(ToSwitch))
			{
				mCurrent=0;
				LoadedRastersList+=mCurrentRasters[mCurrent]->mFilename;
				LoadedRastersList+="'";
				IsInSet = AddRaster(point,LoadedRastersList);
				cout << "Adding for point: " << endl;
					point.Display();
				k = mCurrentRasters.size();
				IsInSet= false;
				while ((k>0)&&(!IsInSet))
				{
					k--;
					IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(point))
							&&(0==mCurrentRasters[k]->mFileSetLevel));
				}
			}

			if(!IsInSet)
			{
				mCurrent=0;
				LoadedRastersList="'";
				k = mCurrentRasters.size();
				IsInSet= false;
				while ((k>0)&&(!IsInSet))
				{
					k--;
					LoadedRastersList+=mCurrentRasters[k]->mFilename;
					LoadedRastersList+="','";
					IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(point));
				}
				if (!IsInSet)
				{
					LoadedRastersList+=mCurrentRasters[mCurrent]->mFilename;
					LoadedRastersList+="'";
					IsInSet = AddRaster(point,LoadedRastersList);
					cout << "Adding for point: " << endl;
					point.Display();
					mCurrent=0;
					k = mCurrentRasters.size();
					IsInSet= false;
					while ((k>0)&&(!IsInSet))
					{
						k--;
						IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(point));
					}
				}

			}
				
			if (IsInSet)
			{
				mCurrent=k;
				profile[j] = mCurrentRasters[mCurrent]->GetValue(point,mSampleMethod);
				mCurrentRasters[mCurrent]->mUsed = true;
				Preferred=(0==mCurrentRasters[mCurrent]->mFileSetLevel);
//				if (mJumped) 
//				{
//					cout << "point:" << mCurrentRasters[mCurrent]->mFilename<< endl;
//					point.Display();
//				}
//				if (mJumped) cout << mCurrentRasters[Current]->mFilename<< endl;
			}
			else
			{
				profile[j]=OUTOFRASTER;
				AllFound = false;
			}
			if ((profile[j] <-440.0)||(profile[j] >8880)||(profile[j]==OUTOFRASTER))
				profile[j]=OUTOFRASTER;
			if (OUTOFRASTER==profile[j])
			{ 
				cout << "foutjie" << endl;
				AllFound=false;
			}
	
		}
		else mCurrentRasters[mCurrent]->mUsed = true;
	}

	OutProfile.SetInterPixelDist(DistRes);
	OutProfile.SetProfile(NumPoints, profile);
//	cProfile Rprofile(NumPoints, profile, DistRes);
	delete [] profile;
	mJumped=false;
	for (k=0;k<mCurrentRasters.size(); k++)
		mCurrentRasters[k]->mUsed = false;
	return AllFound;
//	return Rprofile;
}

//*********************************************************************
// Fixed = false
bool cRasterFileHandler::GetForCoverage(bool Fixed, cGeoP SitePos, double &Range,
					double &DistRes, double &AngRes,
					unsigned &NumAngles, unsigned &NumDistance,
					Float2DArray &Data)
{
	unsigned i,j,k=0, Current=0;
	cGeoP edge, point;
	bool IsInSet = false;
	bool AllFound = true;
	SitePos.SetGeoType(WGS84GC);
	edge.SetGeoType(WGS84GC);
	point.SetGeoType(WGS84GC);
	string LoadedRastersList="'";
	
	cout << "In cRasterFileHandler::GetForCoverage mType =     " << mType << endl;
	if (!Fixed)  //! fixed = false;
	{
		AngRes = (double)360.0/(double)NumAngles; //!分辨率
		NumAngles = (int)floor(360.0/AngRes+0.5); //!角度数
		AngRes = (double)360.0/(double)NumAngles; //!角度分辨率
	}
	else AngRes = 360.0/NumAngles;
	
	IsInSet=false;
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(SitePos))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(SitePos,LoadedRastersList);
	}

	for (j=0; j<12; j++) //Check for preferred files for all the edges.
	{
		edge.FromHere(SitePos,Range,j*30); //基站地址对应半径和旋转方向
		IsInSet = false;
		LoadedRastersList="'";
		for (i=0; i<mCurrentRasters.size(); i++)
		{
            //! 	typedef	vector<pcRaster> VecRaster;
			IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(edge))
					&&(0==mCurrentRasters[i]->mFileSetLevel));
			LoadedRastersList+=mCurrentRasters[i]->mFilename;
			LoadedRastersList+="','";
		}
		if (!IsInSet)
		{
			LoadedRastersList+="'";
			AddRaster(edge,LoadedRastersList);
		}
	}


	if (!Fixed)
	{
		for (i=0; i<mCurrentRasters.size(); i++)
			DistRes = min(DistRes,mCurrentRasters[i]->GetRes());
		NumDistance = (int)ceil(Range/DistRes);
		Range = NumDistance*DistRes;
		NumDistance++;
	}


	if (mCurrentRasters.size()==0)
	{
		string err = "cRasterFileHandler::GetForCoverage; No raster files could be found. Confirm that File-Set Order exist.";
		cout << err << endl;
		cout << " LoadedRastersList:	" << LoadedRastersList << endl;
		cout << " Fixed = " << Fixed << endl;
		QRAP_ERROR(err.c_str());
		return false;
	}
	delete_Float2DArray(Data);
	Data = new_Float2DArray(NumAngles, NumDistance);

//	cout << "NA: " << NumAngles << "	ND: "  << NumDistance << endl;

	k=0; 
	IsInSet = (mCurrentRasters[k]->IsIn(SitePos))&&(0==mCurrentRasters[k]->mFileSetLevel);
	LoadedRastersList="'";
	while ((k<mCurrentRasters.size())&&(!IsInSet))
	{
		LoadedRastersList+=mCurrentRasters[k]->mFilename;
		LoadedRastersList+="','";
		k++;
		IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(SitePos))
					&&(0==mCurrentRasters[k]->mFileSetLevel));
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		IsInSet = AddRaster(SitePos,LoadedRastersList);
		k=0;
		IsInSet = mCurrentRasters[k]->IsIn(SitePos);
		while ((k<mCurrentRasters.size())&&(!IsInSet))
		{
			k++;
			IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(SitePos));
		}
	}

	if (IsInSet)
	{
		Current = k;
		Data[0][0] = mCurrentRasters[Current]->GetValue(SitePos,mSampleMethod);
		mCurrentRasters[Current]->mUsed=true;
	}
	else
	{
		Data[0][0]=0;
		AllFound = false;
	}
	
	if ((Data[0][0]<-440.0)||(Data[0][0]>8880.0)) 
		Data[0][0] = 0;
	// At distance zero the values at all angles will be the same
	// Strictly speaking if the distance is zero the angle is not defined ;-) 
	for (i=1;i<NumAngles;i++)
		Data[i][0]=Data[0][0];
	LoadedRastersList="'";

	bool Preferred=(0==mCurrentRasters[Current]->mFileSetLevel);
	bool ToSwitch=false;
	bool Available=true;

    //-----------------------------
    //! 获取各点数据
	for (i=0; i<NumAngles; i++)
	{
		for(j=1; j<NumDistance; j++)
		{
			point.FromHere(SitePos,j*DistRes,i*AngRes); //! 得到Point
			if (!Preferred)
			{
				ToSwitch=point.Between(mPreferedSetNW,mPreferedSetSE);
				Available=point.Between(mAvailableSetNW,mAvailableSetSE);
			}
			else 
			{	
				ToSwitch=false;
			}

			Data[i][j] = mCurrentRasters[Current]->GetValue(point,mSampleMethod); //! point处的值
			if ((Data[i][j] <-440.0)||(Data[i][j] >8880)||(Data[i][j]==OUTOFRASTER))
				Available=point.Between(mAvailableSetNW,mAvailableSetSE);

			if (((Data[i][j] <-440.0)||(Data[i][j] >8880)||(Data[i][j]==OUTOFRASTER)||(ToSwitch))&&(Available))
			{
//				cout << "OUT: " << OUTOFRASTER << endl;
//				point.SetGeoType(DEG);
//				point.Display();
//				mCurrentRasters[Current]->mUsed = false;
				k = mCurrentRasters.size()-1;
				IsInSet= (mCurrentRasters[k]->IsIn(point))&&(0==mCurrentRasters[k]->mFileSetLevel);
				LoadedRastersList="'";
				while ((k>0)&&(!IsInSet))
				{
					LoadedRastersList+=mCurrentRasters[k]->mFilename;
					LoadedRastersList+="','";
					k--;
					IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(point))
							&&(0==mCurrentRasters[k]->mFileSetLevel));
				}
				if ((!IsInSet)&&(ToSwitch))
				{
					LoadedRastersList+=mCurrentRasters[Current]->mFilename;
					LoadedRastersList+="'";
					IsInSet = AddRaster(point,LoadedRastersList);
					k = mCurrentRasters.size()-1;
					IsInSet= (mCurrentRasters[k]->IsIn(point))
						&&(0==mCurrentRasters[k]->mFileSetLevel);
					while ((k>0)&&(!IsInSet))
					{
						k--;
						IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(point))
								&&(0==mCurrentRasters[k]->mFileSetLevel));
					}
				}

				if(!IsInSet)
				{
					LoadedRastersList="'";
					k = mCurrentRasters.size()-1;
					IsInSet= (mCurrentRasters[k]->IsIn(point));
					while ((k>0)&&(!IsInSet))
					{
						LoadedRastersList+=mCurrentRasters[k]->mFilename;
						LoadedRastersList+="','";
						k--;
						IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(point));
					}
					if (!IsInSet)
					{
						LoadedRastersList+=mCurrentRasters[Current]->mFilename;
						LoadedRastersList+="'";
						IsInSet = AddRaster(point,LoadedRastersList);
						k = mCurrentRasters.size()-1;
						IsInSet= (mCurrentRasters[k]->IsIn(point));
						while ((k>0)&&(!IsInSet))
						{
							k--;
							IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(point));
						}
					}
				}				

				if (IsInSet)
				{
					Current =k;
					Data[i][j] = mCurrentRasters[Current]->GetValue(point,mSampleMethod);
					mCurrentRasters[Current]->mUsed = true;
					Preferred=(0==mCurrentRasters[Current]->mFileSetLevel);
				}
				else
				{
					Data[i][j]=0;
					AllFound = false;
				}
				if (Data[i][j]==OUTOFRASTER) 
					Data[i][j]=0;
			}
			else mCurrentRasters[Current]->mUsed = true;
		}
		if (((double)(25*i)/NumAngles)==((25*i)/NumAngles))
			cout << "Get Raster Data: " << 100*i/NumAngles << endl;
	}

	for (k=0;k<mCurrentRasters.size(); k++)
		mCurrentRasters[k]->mUsed = false;
	return AllFound;
}

//****************************************************************************
bool cRasterFileHandler::GetForDEM(	cGeoP &NW, cGeoP &SE, 
					double &InRes,
					unsigned &Rows, unsigned &Cols, 
					Float2DArray &Data, GeoType ProjIn, bool Fixed) 
{
//	cout << "In GetForDEM" << endl;
	mCurrentRasters.clear();

	cGeoP NE, SW, rNW, rSE, rNE, rSW, Point, tempP,Mid;
	double N,W,S,E,WE, Ydist, Xdist, MidX, MidY, tempD;
	unsigned i,j,k,midRow,midCol;
	int CentMer,Sign=1, central;
	unsigned MeanSize, Current;
	GeoType Proj=ProjIn;
	bool Hemisphere=true;
	bool IsInSet=false;
	double DistRes, NSres, EWres;
	double Res=InRes;
	string LoadedRastersList="'";
	
	NW.SetGeoType(DEG);
	NW.Get(N,W);
	SE.SetGeoType(DEG);
	SE.Get(S,E);
	Mid.Set((N+S)/2,(W+E)/2.0, DEG);
	Hemisphere = Mid.Hemisphere();
//	cout << " In  cRasterFileHandler::GetForDEM.   Mid: " << endl;
//	Mid.Display();
	central=Mid.DefaultCentMer(ProjIn);

	CentMer = central; 
	NW.SetGeoType(ProjIn,central);
	NW.Get(N,W,Proj,CentMer,Hemisphere);
	SE.SetGeoType(ProjIn,central);
	SE.Get(S,E,Proj,CentMer,Hemisphere);
	NE.Set(N,E,ProjIn,central);
	SW.Set(S,W,ProjIn,central);
	Mid.SetGeoType(ProjIn,central);
	Mid.Get(MidY,MidX,Proj,CentMer,Hemisphere);
	tempP.SetGeoType(ProjIn,central);
	Point.SetGeoType(ProjIn,central);
	
	if (Fixed)
	{
		if (ProjIn==DEG)
		{
			NSres=fabs(N-S)/Rows;
			EWres=fabs(E-W)/Cols;
			Sign = 1;
			Res = (NSres+EWres)/2.0;
			DistRes=Res/cDegResT;
		}
		else
		{
			NSres=NW.Distance(SW)/Rows;
			WE = (NW.Distance(NE)+SW.Distance(SE))/2.0;
			EWres = WE/Cols;
			Res = (NSres+EWres)/2.0;
			DistRes = Res;
			if (ProjIn==WGS84GC) Sign = -1;
			else Sign = 1;
		}
	}
	else
	{
		if (ProjIn==DEG)
		{
			Res=cDegResT*InRes;
			Rows = 2*(int)ceil((N-S)/(2.0*Res)+0.5)+1;
			Cols = 2*(int)ceil((E-W)/(2.0*Res)+0.5)+1;
			DistRes = Res*max(min(NW.Distance(NE),SW.Distance(SE))/fabs(E-W),
					min(SE.Distance(NE), SW.Distance(NW))/fabs(N-S));
			Sign = 1;
		}
		else
		{
			DistRes = InRes;
			Res = InRes;
			WE = max(NW.Distance(NE),SW.Distance(SE));
			Rows = 2*(int)ceil(NW.Distance(SW)/(2.0*DistRes)+0.5)+1;
			Cols = 2*(int)ceil(WE/(2.0*DistRes)+0.5)+1;
			if (ProjIn==WGS84GC) Sign = -1;
			else Sign = 1;
		}
		NSres=Res;
		EWres=Res;
	}
	InRes=DistRes;

	if (0==Rows*Cols)
	{
		string err = "Empty Raster ... Rows of Cols is 0 ";
//		QRAP_ERROR(err.c_str());
		cout << "No DEM available for this area" << endl;
		return false;
	}
		
	midRow = (int)(Rows/2.0);
	midCol = (int)(Cols/2.0);
//	cout << Rows << "  " << Cols << endl;
	delete_Float2DArray(Data);
	Data = new_Float2DArray(Rows,Cols);

	Ydist = (double)(0-(int)midRow)*NSres;
	Xdist = (double)(0-(int)midCol)*EWres;
	NW.Set(MidY-Ydist,MidX+Sign*Xdist,ProjIn,central);
//	cout << " In  cRasterFileHandler::GetForDEM.   NW: " << endl;
//	NW.Display();
	Ydist = (double)((int)Rows-1-(int)midRow)*NSres;
	SW.Set(MidY-Ydist,MidX+Sign*Xdist,ProjIn,central);
//	cout << " In  cRasterFileHandler::GetForDEM.   SW: " << endl;
//	SW.Display();
	Xdist = (double)((int)Cols-1-(int)midCol)*EWres;
	SE.Set(MidY-Ydist,MidX+Sign*Xdist,ProjIn,central);
//	cout << " In  cRasterFileHandler::GetForDEM.   SE: " << endl;
//	SE.Display();
	Ydist = (double)(0-(int)midRow)*NSres;
	NE.Set(MidY-Ydist,MidX+Sign*Xdist,ProjIn,central);
//	cout << " In  cRasterFileHandler::GetForDEM.   NE: " << endl;
//	NE.Display();

//	cout << "In cRasterFileHandler::GetForDEM.  Resolution: " << Res << endl;
//	cout << "In cRasterFileHandler::GetForDEM.  midRow: " << midRow << endl;
//	cout << "In cRasterFileHandler::GetForDEM.  midCol: " << midCol << endl;

	for(i=0; i<Rows; i++)
		for (j=0; j<Cols; j++)
			Data[i][j]=OUTOFRASTER;

	IsInSet=false;
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(Mid))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(Mid,LoadedRastersList);
	}


	IsInSet=false;
	LoadedRastersList="'";
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(SE))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(SE,LoadedRastersList);
	}	
	
	IsInSet=false;
	LoadedRastersList="'";
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(NE))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(NE,LoadedRastersList);
	}

	IsInSet=false;
	LoadedRastersList="'";
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(SW))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(SW,LoadedRastersList);
	}

	IsInSet=false;
	LoadedRastersList="'";
	for (i=0; i<mCurrentRasters.size(); i++)
	{
		IsInSet = IsInSet || ((mCurrentRasters[i]->IsIn(NW))
				&&(0==mCurrentRasters[i]->mFileSetLevel));
		LoadedRastersList+=mCurrentRasters[i]->mFilename;
		LoadedRastersList+="','";
	}
	if (!IsInSet)
	{
		LoadedRastersList+="'";
		AddRaster(NW,LoadedRastersList);
	}

	IsInSet=false;
	LoadedRastersList="'";

	if (mCurrentRasters.size()!=0)
	{
		Current=0;
		MeanSize=mCurrentRasters[Current]->GetSize();
	}
	else 
	{
		string err = "No DEM available for this area ";
//		QRAP_ERROR(err.c_str());
		cout << "No DEM available for this area" << endl;
		return false;
	}

	bool Preferred=(0==mCurrentRasters[Current]->mFileSetLevel);
	bool ToSwitch=false;
	bool Available=true;	
	for (i=0; (i<Rows); i++)
	{
//		Sign = i-midRow;
//		Sign = (int)Sign/fabs(Sign);
		Ydist = (double)((int)i-(int)midRow)*NSres;
		for (j=0; (j<Cols); j++)
		{
//			tempP.FromHere(Mid,Sign*(i-midRow)*DistRes,(double)(1.0+Sign)*90.0);
//			Sign = j-midCol;
//			Sign = (int)Sign/fabs(Sign);
			Xdist = (double)((int)j-(int)midCol)*EWres;
			Point.Set(MidY-Ydist,MidX+Sign*Xdist,ProjIn,central);
//			Point.FromHere(tempP,Sign*(j-midCol)*DistRes,90+(double)(1.0-Sign)*90.0);
			if (!Preferred)
			{
				ToSwitch=Point.Between(mPreferedSetNW,mPreferedSetSE);
				Available=Point.Between(mAvailableSetNW,mAvailableSetSE);
			}
			else 
			{	
				ToSwitch=false;
			}
			Data[i][j]=mCurrentRasters[Current]->GetValue(Point,mSampleMethod);
			if ((Data[i][j] == OUTOFRASTER)||(Data[i][j]<-440)||(Data[i][j]>8880))
				Available=Point.Between(mAvailableSetNW,mAvailableSetSE);
			tempD = Data[i][j];
			if (((Data[i][j] == OUTOFRASTER)||(Data[i][j]<-440)||(Data[i][j]>8880)||(ToSwitch))&&(Available))
			{
//				mCurrentRasters[Current]->mUsed = false;
				k=0;
				IsInSet= (mCurrentRasters[k]->IsIn(Point))&&(0==mCurrentRasters[k]->mFileSetLevel);
				LoadedRastersList="'";
				while ((k<mCurrentRasters.size()-1)&&(!IsInSet))
				{
					LoadedRastersList+=mCurrentRasters[k]->mFilename;
					LoadedRastersList+="','";
					k++;
					IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(Point))
								&&(0==mCurrentRasters[k]->mFileSetLevel));
				}
				if ((!IsInSet)&&(ToSwitch))
				{
					LoadedRastersList+=mCurrentRasters[Current]->mFilename;
					LoadedRastersList+="'";
					IsInSet = AddRaster(Point,LoadedRastersList);
					k=mCurrentRasters.size()-1;
					IsInSet= (mCurrentRasters[k]->IsIn(Point))&&(0==mCurrentRasters[k]->mFileSetLevel);
					while ((k>0)&&(!IsInSet))
					{
						k--;
						IsInSet = IsInSet||((mCurrentRasters[k]->IsIn(Point))
								&&(0==mCurrentRasters[k]->mFileSetLevel));
					}
				}
				if(!IsInSet)
				{
					LoadedRastersList="'";
					k = mCurrentRasters.size()-1;
					IsInSet= (mCurrentRasters[k]->IsIn(Point));
					while ((k>0)&&(!IsInSet))
					{
						LoadedRastersList+=mCurrentRasters[k]->mFilename;
						LoadedRastersList+="','";
						k--;
						IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(Point));
					}
					if (!IsInSet)
					{
						LoadedRastersList+=mCurrentRasters[Current]->mFilename;
						LoadedRastersList+="'";
						IsInSet = AddRaster(Point,LoadedRastersList);
						k = mCurrentRasters.size()-1;
						IsInSet= (mCurrentRasters[k]->IsIn(Point));
						while ((k>0)&&(!IsInSet))
						{
							k--;
							IsInSet = IsInSet||(mCurrentRasters[k]->IsIn(Point));
						}
					}
				}				

				
				if (IsInSet)
				{
					Current = k;
					Data[i][j] = mCurrentRasters[k]->GetValue(Point,mSampleMethod);
					mCurrentRasters[k]->mUsed = true;
					Preferred=(0==mCurrentRasters[Current]->mFileSetLevel);
				}
				else Data[i][j]=0;
			}
			else mCurrentRasters[Current]->mUsed = true;
		}
	}
	for (k=0;k<mCurrentRasters.size(); k++)
		mCurrentRasters[k]->mUsed = false;

//	cout << "Out GetForDEM" << endl;
	return true;
}

//****************************************************************************
bool cRasterFileHandler::AddRaster(cGeoP point, string LoadedRastersNames)
{
	bool RasterFound = false;
	unsigned i=0;
	static bool NewFound;
	cGeoP NotFoundPoint;
	static int NotFoundCount;
	
	string Directory, FileName, PointString, Type, proj4string, projection;
	FileType filetype;
	pqxx::result r;
	string query;
	char *temp;
	temp = new char[33];
	char *temp2;
	temp2 = new char[33];
	double lat,lon;
	double N, W, S, E, Nm, Wm, Sm, Em;
	int centmer;
	GeoType GeoProj;
	point.SetGeoType(DEG);
	point.Get(lat,lon);
	gcvt(lon,9,temp);
	gcvt(lat,9,temp2);
	PointString = "POINT'(";
	PointString += temp2;
	PointString += ",";
	PointString += temp;
	PointString += ")'"; 
//	cout << "cRasterFileHandler::AddRaster: mFileSetOrder.size() = " << mFileSetOrder.size() << endl;
	i=0;
	
	while((i<mFileSetOrder.size())&&(!(RasterFound)))
	{
		query = "SELECT distinct filename,location,fileformat, projection, proj4string, centmer ";
		query += "FROM sourcefiles CROSS JOIN filesets ";
		query += "WHERE filesetkey=filesets.id AND filesets.id=";
		gcvt(mFileSetOrder[i],8,temp);
		query += temp; 
//		query += " AND sourcefiles.machineid=";
//		gcvt(gDb.globalMachineID,8,temp);
//		query += temp; 
		query += " AND filename NOT IN ("+LoadedRastersNames;
		query += ") AND "+ PointString + " <@"+"areasquare;";
		
//		cout << query << endl;				
		if(!gDb.PerformRawSql(query))
		{
			string err = "In cRasterFileHandler::AddRaster. Database Select for RasterFile failed. Query: ";
			err += query;
			QRAP_WARN(err.c_str());
		}
		else
		{
			gDb.GetLastResult(r);
			if (r.size())
			for (unsigned j=0; j<r.size(); j++)
			{
				if (!NewFound)
				{				
					NewFound = true;
//					cout << " mFileSetOrder.size() = " 
//						<< mFileSetOrder.size() << "	i=" << i << endl;
//					cout << "Not finding point ..."; 
//					NotFoundPoint.Display();
					cout << "Found point ... ";
					point.Display();
				}
				NotFoundCount = 0;
				RasterFound = true;
				FileName = r[j]["filename"].c_str();
				Directory = r[j]["location"].c_str();
				Type = r[j]["fileformat"].c_str();

				if (Type=="BINFILE") 		filetype=BINFILE;
				else if (Type=="NESFILE") 	filetype=NESFILE;
				else if (Type=="GRASSFILE")	filetype=GRASSFILE;
				else if (Type=="GDALFILE")	filetype=GDALFILE;
				else if (Type=="ORTFILE")	filetype=ORTFILE;
				proj4string = r[j]["proj4string"].c_str();
				centmer = atoi(r[j]["centmer"].c_str());
				projection = r[j]["projection"].c_str();
				if (projection=="DEG") 		GeoProj=DEG;
				else if (projection=="WGS84GC") GeoProj=WGS84GC;
				else if (projection=="UTM")	GeoProj=UTM;
				else GeoProj=NDEF;
//				cout << FileName <<  endl;
				cRaster* New = new cRaster(Directory, FileName, filetype, GeoProj,proj4string, centmer);
				New->mUsed=true;
				New->mFileSetLevel=i;
//				cout << "FileSetLevel = " << i << endl;
				mCurrentRasters.push_back(New);
				if (0==i)
				{
					if (!New->mNW.Between(mPreferedSetNW,mPreferedSetSE))
					{
						New->mNW.SetGeoType(DEG);
						New->mNW.Get(N,W);
						mPreferedSetNW.Get(Nm,Wm);
						mPreferedSetNW.Set(max(N,Nm),min(W,Wm));
					}
					if (!New->mSE.Between(mPreferedSetNW,mPreferedSetSE))
					{
						New->mSE.SetGeoType(DEG);
						New->mSE.Get(S,E);
						mPreferedSetSE.Get(Sm,Em);
						mPreferedSetSE.Set(min(S,Sm),max(E,Em));
					}
				}
				if (!New->mNW.Between(mAvailableSetNW,mAvailableSetSE))
				{
					New->mNW.SetGeoType(DEG);
					New->mNW.Get(N,W);
					mAvailableSetNW.Get(Nm,Wm);
					mAvailableSetNW.Set(max(N,Nm),min(W,Wm));

				}
				if (!New->mSE.Between(mAvailableSetNW,mAvailableSetSE))
				{
					New->mSE.SetGeoType(DEG);
					New->mSE.Get(S,E);
					mAvailableSetSE.Get(Sm,Em);
					mAvailableSetSE.Set(min(S,Sm),max(E,Em));
				}

//				cout << "NumRastersLoaded: " << mCurrentRasters.size()  << endl << endl;
			} // if r.size()
			else
			{
				if (NewFound)
				{
//					cout << " mFileSetOrder.size() = "; 
//					cout << mFileSetOrder.size() << "	i=" << i << endl;
//					cout << "Not finding point ... ";
//					point.Display();
					NotFoundCount = 0;
					NewFound = false;
				}
				else NotFoundPoint = point; 
				if ((NotFoundCount>100000)||(NotFoundCount<1))
				{
//					cout << " mFileSetOrder.size() = "; 
//					cout << mFileSetOrder.size() << "	i=" << i << endl;
					cout << "Not finding point ... ";
					point.Display();
					NotFoundCount = 0;
					NewFound = false;
				}
				NotFoundCount++;
			}
//				cout << query << endl;	
//				cout << "cRasterFileHandler::AddRaster: r.size() = " << r.size() << endl;
			
		} // else !gDb->PerformRawSq	// Select filename  and Directory from files 
		i++;
	}
	
	if ((RasterFound)&&(mCurrentRasters.size()>MAX_NUM_RASTERS))
	{	
		for (i=0; i<mCurrentRasters.size(); i++)
		{
			if (!mCurrentRasters[i]->mUsed)
			{
//				cout << "Removing " << mCurrentRasters[i]->mFilename << endl;
				delete mCurrentRasters[i];
				mCurrentRasters.erase(mCurrentRasters.begin()+i);
			}
		}
		for (i=0; i<mCurrentRasters.size(); i++)
			mCurrentRasters[i]->mUsed = false;
	}

	cout << "Available: " << endl;
	mAvailableSetNW.Display();
	mAvailableSetSE.Display();
	
	delete [] temp;
	delete [] temp2;
	return RasterFound;
}
