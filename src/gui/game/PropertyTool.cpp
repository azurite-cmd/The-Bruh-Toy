#include "Tool.h"

#include "prefs/GlobalPrefs.h"
#include "Menu.h"
#include "Format.h"

#include "gui/game/GameModel.h"
#include "gui/Style.h"
#include "gui/game/Brush.h"
#include "gui/interface/Window.h"
#include "gui/interface/Button.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/DropDown.h"
#include "gui/dialogues/ErrorMessage.h"

#include "simulation/GOLString.h"
#include "simulation/BuiltinGOL.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"

#include "graphics/Graphics.h"

#include "Config.h"
#include <iostream>
#include <SDL.h>

class PropertyWindow: public ui::Window
{
public:
	ui::DropDown * property;
	ui::Textbox * textField;
	PropertyTool * tool;
	Simulation *sim;
	std::vector<StructProperty> properties;
	PropertyWindow(PropertyTool *tool_, Simulation *sim);
	void SetProperty(bool warn);
	void OnDraw() override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnTryExit(ExitMethod method) override;
	virtual ~PropertyWindow() {}
};

PropertyWindow::PropertyWindow(PropertyTool * tool_, Simulation *sim_):
ui::Window(ui::Point(-1, -1), ui::Point(200, 87)),
tool(tool_),
sim(sim_)
{
	properties = Particle::GetProperties();

	ui::Label * messageLabel = new ui::Label(ui::Point(4, 5), ui::Point(Size.X-8, 14), "Edit property");
	messageLabel->SetTextColour(style::Colour::InformationTitle);
	messageLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	messageLabel->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	AddComponent(messageLabel);

	ui::Button * okayButton = new ui::Button(ui::Point(0, Size.Y-17), ui::Point(Size.X, 17), "OK");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	okayButton->SetActionCallback({ [this] {
		if (textField->GetText().length())
		{
			CloseActiveWindow();
			SetProperty(true);
			SelfDestruct();
		}
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	property = new ui::DropDown(ui::Point(8, 25), ui::Point(Size.X-16, 16));
	property->SetActionCallback({ [this] { FocusComponent(textField); } });
	AddComponent(property);
	for (int i = 0; i < int(properties.size()); i++)
	{
		property->AddOption(std::pair<String, int>(properties[i].Name.FromAscii(), i));
	}

	auto &prefs = GlobalPrefs::Ref();
	property->SetOption(prefs.Get("Prop.Type", 0));

	textField = new ui::Textbox(ui::Point(8, 46), ui::Point(Size.X-16, 16), "", "[value]");
	textField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	textField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	textField->SetText(prefs.Get("Prop.Value", String("")));
	AddComponent(textField);
	FocusComponent(textField);
	SetProperty(false);

	MakeActiveWindow();
}

void PropertyWindow::SetProperty(bool warn)
{
	tool->validProperty = false;
	if(property->GetOption().second!=-1 && textField->GetText().length() > 0)
	{
		tool->validProperty = true;
		String value = textField->GetText().ToUpper();
		try {
			switch(properties[property->GetOption().second].Type)
			{
				case StructProperty::Integer:
				case StructProperty::ParticleType:
				{
					int v;
					if(value.length() > 2 && value.BeginsWith("0X"))
					{
						//0xC0FFEE
						v = value.Substr(2).ToNumber<unsigned int>(Format::Hex());
					}
					else if(value.length() > 1 && value.BeginsWith("#"))
					{
						//#C0FFEE
						v = value.Substr(1).ToNumber<unsigned int>(Format::Hex());
					}
					else
					{
						// Try to parse as particle name
						v = sim->GetParticleType(value.ToUtf8());

						// Try to parse special GoL rules
						if (v == -1 && properties[property->GetOption().second].Name == "ctype")
						{
							if (value.length() > 1 && value.BeginsWith("B") && value.Contains("/"))
							{
								v = ParseGOLString(value);
								if (v == -1)
								{
									class InvalidGOLString : public std::exception
									{
									};
									throw InvalidGOLString();
								}
							}
							else
							{
								v = sim->GetParticleType(value.ToUtf8());
								if (v == -1)
								{
									for (auto *elementTool : tool->gameModel.GetMenuList()[SC_LIFE]->GetToolList())
									{
										if (elementTool && elementTool->Name == value)
										{
											v = ID(elementTool->ToolID);
											break;
										}
									}
								}
							}
						}

						// Parse as plain number
						if (v == -1)
						{
							v = value.ToNumber<int>();
						}
					}

					if (properties[property->GetOption().second].Name == "type" && (v < 0 || v >= PT_NUM || !sim->elements[v].Enabled))
					{
						tool->validProperty = false;
						if (warn)
							new ErrorMessage("Could not set property", "Invalid particle type");
						return;
					}
					if constexpr (DEBUG)
					{
						std::cout << "Got int value " << v << std::endl;
					}
					tool->propValue.Integer = v;
					break;
				}
				case StructProperty::UInteger:
				{
					unsigned int v;
					if(value.length() > 2 && value.BeginsWith("0X"))
					{
						//0xC0FFEE
						v = value.Substr(2).ToNumber<unsigned int>(Format::Hex());
					}
					else if(value.length() > 1 && value.BeginsWith("#"))
					{
						//#C0FFEE
						v = value.Substr(1).ToNumber<unsigned int>(Format::Hex());
					}
					else
					{
						v = value.ToNumber<unsigned int>();
					}
					if constexpr (DEBUG)
					{
						std::cout << "Got uint value " << v << std::endl;
					}
					tool->propValue.UInteger = v;
					break;
				}
				case StructProperty::Float:
				{
					if (properties[property->GetOption().second].Name == "temp")
						tool->propValue.Float = format::StringToTemperature(value, tool->gameModel.GetTemperatureScale());
					else
						tool->propValue.Float = value.ToNumber<float>();
				}
					break;
				default:
					tool->validProperty = false;
					if (warn)
						new ErrorMessage("Could not set property", "Invalid property");
					return;
			}
			tool->propOffset = properties[property->GetOption().second].Offset;
			tool->propType = properties[property->GetOption().second].Type;
			tool->changeType = properties[property->GetOption().second].Name == "type";
		} catch (const std::exception& ex) {
			tool->validProperty = false;
			if (warn)
				new ErrorMessage("Could not set property", "Invalid value provided");
			return;
		}
		{
			auto &prefs = GlobalPrefs::Ref();
			Prefs::DeferWrite dw(prefs);
			prefs.Set("Prop.Type", property->GetOption().second);
			prefs.Set("Prop.Value", textField->GetText());
		}
	}
}

void PropertyWindow::OnTryExit(ExitMethod method)
{
	CloseActiveWindow();
	SelfDestruct();
}

void PropertyWindow::OnDraw()
{
	Graphics * g = GetGraphics();

	g->clearrect(Position.X-2, Position.Y-2, Size.X+3, Size.Y+3);
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 200, 200, 200, 255);
}

void PropertyWindow::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_UP)
		property->SetOption(property->GetOption().second-1);
	else if (key == SDLK_DOWN)
		property->SetOption(property->GetOption().second+1);
}

void PropertyTool::OpenWindow(Simulation *sim)
{
	new PropertyWindow(this, sim);
}

void PropertyTool::SetProperty(Simulation *sim, ui::Point position)
{
	if(position.X<0 || position.X>XRES || position.Y<0 || position.Y>YRES || !validProperty)
		return;
	int i = sim->pmap[position.Y][position.X];
	if(!i)
		i = sim->photons[position.Y][position.X];
	if(!i)
		return;

	if (changeType)
	{
		sim->part_change_type(ID(i), int(sim->parts[ID(i)].x+0.5f), int(sim->parts[ID(i)].y+0.5f), propValue.Integer);
		return;
	}

	switch (propType)
	{
		case StructProperty::Float:
			*((float*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.Float;
			break;
		case StructProperty::ParticleType:
		case StructProperty::Integer:
			*((int*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.Integer;
			break;
		case StructProperty::UInteger:
			*((unsigned int*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.UInteger;
			break;
		default:
			break;
	}
}

void PropertyTool::Draw(Simulation *sim, Brush const &cBrush, ui::Point position)
{
	for (ui::Point off : cBrush)
	{
		ui::Point coords = position + off;
		if (coords.X >= 0 && coords.Y >= 0 && coords.X < XRES && coords.Y < YRES)
			SetProperty(sim, coords);
	}
}

void PropertyTool::DrawLine(Simulation *sim, Brush const &cBrush, ui::Point position, ui::Point position2, bool dragging)
{
	int x1 = position.X, y1 = position.Y, x2 = position2.X, y2 = position2.Y;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy, rx = cBrush.GetRadius().X, ry = cBrush.GetRadius().Y;
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			Draw(sim, cBrush, ui::Point(y, x));
		else
			Draw(sim, cBrush, ui::Point(x, y));
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(rx+ry) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					Draw(sim, cBrush, ui::Point(y, x));
				else
					Draw(sim, cBrush, ui::Point(x, y));
			}
			e -= 1.0f;
		}
	}
}

void PropertyTool::DrawRect(Simulation *sim, Brush const &cBrush, ui::Point position, ui::Point position2)
{
	int x1 = position.X, y1 = position.Y, x2 = position2.X, y2 = position2.Y;
	int i, j;
	if (x1>x2)
	{
		i = x2;
		x2 = x1;
		x1 = i;
	}
	if (y1>y2)
	{
		j = y2;
		y2 = y1;
		y1 = j;
	}
	for (j=y1; j<=y2; j++)
		for (i=x1; i<=x2; i++)
			SetProperty(sim, ui::Point(i, j));
}

void PropertyTool::DrawFill(Simulation *sim, Brush const &cBrush, ui::Point position)
{
	if (validProperty)
		sim->flood_prop(position.X, position.Y, propOffset, propValue, propType);
}
