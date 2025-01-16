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

namespace CamRover.ControllerApp.Types
{

	internal static class BindableObjectExtensions
	{
		public static void Dispatch( this BindableObject o, Action a )
		{
			if (o.Dispatcher.IsDispatchRequired)
			{
				o.Dispatcher.Dispatch( a );
			}
			else
			{
				a();
			}
		}


		public static async Task Dispatch( this BindableObject o, Func<Task> a )
		{
			if (o.Dispatcher.IsDispatchRequired)
			{
				await o.Dispatcher.DispatchAsync( a );
			}
			else
			{
				await a();
			}
		}


		public static async Task DispatchAsync( this BindableObject o, Action a )
		{
			if (o.Dispatcher.IsDispatchRequired)
			{
				await o.Dispatcher.DispatchAsync( a );
			}
			else
			{
				a();
			}
		}
	}

}
