/*
 *	Copyright (c) 2025 Denis Rozhkov <denis@rozhkoff.com>
 *	This file is part of cam-rover.
 *
 *	cam-rover is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	cam-rover is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 *	Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along with
 *	cam-rover. If not, see <https://www.gnu.org/licenses/>.
 */

// SPDX-FileCopyrightText: 2025 Denis Rozhkov <denis@rozhkoff.com>
// SPDX-License-Identifier: GPL-3.0-or-later

using CamRover.ControllerApp.Models;

namespace CamRover.ControllerApp.ViewModels
{

	public class SettingsViewModel : ControllerAppViewModel
	{
		CommModel m_comm;

		public uint MoveSpeedIncrement
		{
			get
			{
				return m_comm.MoveSpeedIncrement;
			}
			set
			{
				m_comm.MoveSpeedIncrement = value;
				RaisePropertyChanged();
			}
		}


		public SettingsViewModel( CommModel comm )
		{
			m_comm = comm;
		}
	}

}
