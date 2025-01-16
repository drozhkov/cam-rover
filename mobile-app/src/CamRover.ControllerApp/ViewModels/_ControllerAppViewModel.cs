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

using System.ComponentModel;
using System.Linq.Expressions;
using System.Runtime.CompilerServices;

namespace CamRover.ControllerApp.ViewModels
{

	public class ControllerAppViewModel : INotifyPropertyChanged
	{
		public event PropertyChangedEventHandler? PropertyChanged;


		public void RaisePropertyChanged( [CallerMemberName] string? propertyName = null )
		{
			this.PropertyChanged?.Invoke( this, new PropertyChangedEventArgs( propertyName ) );
		}
	}


	public static class ControllerAppViewModelX
	{
		public static void RaisePropertyChanged<T, TProperty>( this T m, Expression<Func<T, TProperty>> e ) where T : ControllerAppViewModel
		{
			m.RaisePropertyChanged( (e.Body as MemberExpression)?.Member.Name );
		}
	}

}
