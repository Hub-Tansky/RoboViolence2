/*
	Copyright 2012 bitHeads inc.

	This file is part of the BaboViolent 2 source code.

	The BaboViolent 2 source code is free software: you can redistribute it and/or 
	modify it under the terms of the GNU General Public License as published by the 
	Free Software Foundation, either version 3 of the License, or (at your option) 
	any later version.

	The BaboViolent 2 source code is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with the 
	BaboViolent 2 source code. If not, see http://www.gnu.org/licenses/.
*/

#include "CThread.h"

CThread::CThread()
	: mArg(0), mIsRunning(false)
{
}

CThread::~CThread()
{
	if (mThread.joinable())
		mThread.join();
}

bool CThread::start(void * pArg, int pThreadPriority)
{
	(void)pThreadPriority;
	if (mIsRunning)
		return false;
	if (mThread.joinable())
		mThread.join(); // a previous run has finished

	arg(pArg); // store user data
	mIsRunning = true;
	try
	{
		mThread = std::thread([this]() { run(this->arg()); });
	}
	catch (...)
	{
		mIsRunning = false;
		return false;
	}
	return true;
}

void CThread::run(void * pArg)
{
	setup();
	execute(pArg);
	mIsRunning = false;
}

void CThread::setup()
{
	// Do any setup here
}
