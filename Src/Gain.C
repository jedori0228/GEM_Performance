#include "Gain.h"

ClassImp(Gain);

//////////

Gain::Gain(const TString& a_path, const TString& a_target)
{
  path = a_path;
  target = a_target;

  fout = new TFile(target+".root", "RECREATE");

  Read_Data();
  Calculate_Gain();
  Fit();
}//Gain::Gain()

//////////

Gain::~Gain()
{
  gr_rate.SetName("Gr_HV_Current_Rate");
  gr_rate.SetTitle("HV_Current vs Rate");
  gr_rate.Write();

  gr_gain.SetName("Gr_HV_Current_Gain");
  gr_gain.SetTitle("HV_Current vs Gain");
  gr_gain.Write();

  fit_gain.SetName("Fit_HV_Current_Gain");
  fit_gain.SetTitle("Fit: HV_Current vs Gain");
  fit_gain.Write();

  gr_gain_conf.SetName("Conf_HV_Current_Gain");
  gr_gain_conf.SetTitle("Confidence Level: HV_Current vs Gain");
  gr_gain_conf.Write();
  
  fout->Close();
}//Gain::~Gain()

//////////

void Gain::Calculate_Gain()
{
  Data data = vec_data[13];
  
  Int_t count_plateau = data.count_on - data.count_off;
  Float_t count_error_plateau = Sqrt(data.count_on + data.count_off);

  Float_t rate_plateau = count_plateau/60.;
  Float_t rate_error_plateau = count_error_plateau/60.;
  
  Int_t n_point = vec_data.size();
  for(Int_t i=0; i<n_point; i++)
    {
      Data data = vec_data[i];

      Int_t hv_current = data.hv_current;
      
      Int_t count = data.count_on - data.count_off;
      Float_t count_error = Sqrt(data.count_on + data.count_off);

      Float_t rate = count/60.;
      Float_t rate_error = count_error/60.;
      
      Float_t ro_current = data.ro_current_on - data.ro_current_off;
      Float_t ro_current_error = Sqrt(Power(data.ro_current_error_on, 2.) + Power(data.ro_current_error_off, 2));

      Float_t gain = ro_current/rate_plateau/ELECTRON_CHARGE/N_PRIMARY_ELECTRON_FE55;

      Float_t gain_error = Power(ro_current_error/ELECTRON_CHARGE/rate_plateau/N_PRIMARY_ELECTRON_FE55, 2.);
      gain_error += Power(ro_current*((rate_error_plateau/rate_plateau)+(N_PRIMARY_ELECTRON_ERROR_FE55/N_PRIMARY_ELECTRON_FE55))/ELECTRON_CHARGE/rate_plateau/N_PRIMARY_ELECTRON_FE55, 2.);
      gain_error = Sqrt(gain_error);
      
      gr_rate.SetPoint(i, hv_current, rate);
      gr_rate.SetPointError(i, 0, rate_error);
            
      gr_gain.SetPoint(i, hv_current, gain);
      gr_gain.SetPointError(i, 0, gain_error);
    }
  
  return;
}//void Gain::Calculate_Gain()

//////////

void Gain::Fit()
{
  gr_gain.Fit("expo", "S", "", 540, 690);
  fit_gain = *((TF1*)gr_gain.GetListOfFunctions()->FindObject("expo"));

  Int_t n_point = 100;
  Float_t range_lower = 550;
  Float_t range_upper = 900;
  
  for(Int_t i=0; i<n_point; i++)
    {
      Double_t x = x = (range_upper-range_lower)/(Double_t)n_point*(Double_t)i + range_lower;
      Double_t y = fit_gain.Eval(x);

      gr_gain_conf.SetPoint(i, x, y);
    }
  
  (TVirtualFitter::GetFitter())->GetConfidenceIntervals(&gr_gain_conf, 0.68);
  
  return;
}//void Gain::Fit()

//////////

void Gain::Read_Data()
{
  TString count_data_path = path + "/Data/" + target + "/Data.csv";
  
  ifstream count_data;
  count_data.open(count_data_path.Data());
  
  string buf;
  
  getline(count_data, buf);
  while(!count_data.eof())
    {
      getline(count_data, buf);
      if(buf.compare("")==0) break;
      
      stringstream ss;
      ss.str(buf);

      Data data;
      
      getline(ss, buf, ',');
      data.voltage = stoi(buf, nullptr);
      
      getline(ss, buf, ',');
      data.hv_current = stoi(buf, nullptr);

      getline(ss, buf, ',');
      data.count_off = stoi(buf, nullptr);

      getline(ss, buf, ',');
      data.count_on = stoi(buf, nullptr);

      TString ro_data_off_path = path + "/Data/" + target + "/Off/" + data.hv_current + ".txt";
      
      Read_RO ro_off(ro_data_off_path);
      data.ro_current_off = ro_off.Get_Mean();
      data.ro_current_error_off = ro_off.Get_Mean_Error();

      TString ro_data_on_path = path + "/Data/" + target + "/On/" + data.hv_current + ".txt";

      Read_RO ro_on(ro_data_on_path);
      data.ro_current_on = ro_on.Get_Mean();
      data.ro_current_error_on = ro_on.Get_Mean_Error();

      vec_data.push_back(data);
    }
  return;
}//void Gain::Read_Data()

//////////
