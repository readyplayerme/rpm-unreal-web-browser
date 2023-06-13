// Copyright © 2021++ Ready Player Me

#include "RpmWebBrowserWidget.h"
#include "SWebBrowser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "WebMessage.h"
#include "WebBrowserEvents.h"

static const FString LinkObjectName = TEXT("rpmlinkobject");
static const FString JavascriptPath = TEXT("RpmWebBrowser/Scripts/RpmFrameSetup.js");

static const TCHAR* ClearCacheParam = TEXT("clearCache");
static const TCHAR* QuickStartParam = TEXT("quickStart");
static const TCHAR* FullBodyParam = TEXT("bodyType=fullbody");
static const TCHAR* HalfBodyParam = TEXT("bodyType=halfbody");
static const TCHAR* SelectBodyTypeParam = TEXT("selectBodyType");
static const TCHAR* GenderMaleParam = TEXT("gender=male");
static const TCHAR* GenderFemaleParam = TEXT("gender=female");
static const TCHAR* LoginTokenParam = TEXT("token");
static const TCHAR* FrameApiParam = TEXT("frameApi");

static const TMap<ELanguage, FString> LANGUAGE_TO_STRING =
{
	{ELanguage::En, "en"},
	{ELanguage::EnIe, "en-IE"},
	{ELanguage::De, "de"},
	{ELanguage::Fr, "fr"},
	{ELanguage::Es, "es"},
	{ELanguage::EsMx, "es-MX"},
	{ELanguage::Pt, "pt"},
	{ELanguage::PtBr, "pt-BR"},
	{ELanguage::It, "it"},
	{ELanguage::Tr, "tr"},
	{ELanguage::Jp, "jp"},
	{ELanguage::Kr, "kr"},
	{ELanguage::Ch, "ch"}
};

void URpmWebBrowserWidget::SetupBrowser()
{
	BindBrowserToObject();
	const FString Path = FPaths::ProjectPluginsDir() / JavascriptPath;

	FString RpmSetupJavascript;
	if (FPaths::FileExists(Path))
	{
		FFileHelper::LoadFileToString(RpmSetupJavascript, *Path);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Script file not found: %s"), *Path);
		return;
	}

	ExecuteJavascript(RpmSetupJavascript);
}

void URpmWebBrowserWidget::BindBrowserToObject()
{
	this->Rename(*LinkObjectName);
	WebBrowserWidget->BindUObject(LinkObjectName, this);
}

void URpmWebBrowserWidget::HandleEvents(const FString& JsonResponse) const
{
	const FWebMessage WebMessage = FWebViewEvents::ConvertJsonStringToWebMessage(JsonResponse);
	if (WebMessage.EventName == FWebViewEvents::USER_SET)
	{
		if (OnUserSet.IsBound())
		{
			OnUserSet.Broadcast(WebMessage.GetId());
		}
	}
	else if (WebMessage.EventName == FWebViewEvents::USER_AUTHORIZED)
	{
		if (OnUserAuthorized.IsBound())
		{
			OnUserAuthorized.Broadcast(WebMessage.GetUserId());
		}
	}
	else if (WebMessage.EventName == FWebViewEvents::AVATAR_EXPORT)
	{
		if (OnAvatarExported.IsBound())
		{
			OnAvatarExported.Broadcast(WebMessage.GetUrl());
		}
	}
	else if (WebMessage.EventName == FWebViewEvents::ASSET_UNLOCK)
	{
		if (OnAssetUnlock.IsBound())
		{
			OnAssetUnlock.Broadcast(WebMessage.GetAssetRecord());
		}
	}
	UE_LOG(LogTemp, Log, TEXT("WebEvent: %s"), *WebMessage.EventName);
}

void URpmWebBrowserWidget::AddBodyTypeParam(TArray<FString>& Params) const
{
	switch (SelectBodyType)
	{
	case ESelectBodyType::FullBody:
		Params.Add(FullBodyParam);
		break;
	case ESelectBodyType::HalfBody:
		Params.Add(HalfBodyParam);
		break;
	case ESelectBodyType::Select:
		Params.Add(SelectBodyTypeParam);
		break;
	default:
		break;
	}
}

void URpmWebBrowserWidget::AddGenderParam(TArray<FString>& Params) const
{
	switch (SelectGender)
	{
	case ESelectGender::Male:
		Params.Add(GenderMaleParam);
		break;
	case ESelectGender::Female:
		Params.Add(GenderFemaleParam);
		break;
	default:
		break;
	}
}

FString URpmWebBrowserWidget::BuildUrl(const FString& LoginToken) const
{
	TArray<FString> Params;
	if(!LoginToken.IsEmpty())
	{
		Params.Add(LoginTokenParam + '=' + LoginToken);
	}
	
	Params.Add(FrameApiParam);
	if (bClearCache)
	{
		Params.Add(ClearCacheParam);
	}
	if (bQuickStart)
	{
		Params.Add(QuickStartParam);
	}
	AddBodyTypeParam(Params);
	AddGenderParam(Params);

	FString UrlQueryStr;
	if (!Params.IsEmpty())
	{
		UrlQueryStr = "?" + FString::Join(Params, TEXT("&"));
	}
	FString LanguageStr;
	if (Language != ELanguage::Default)
	{
		LanguageStr = "/" + LANGUAGE_TO_STRING[Language];
	}

	return FString::Printf(
		TEXT("https://%s.readyplayer.me%s/avatar%s"), *PartnerDomain, *LanguageStr, *UrlQueryStr);
}

void URpmWebBrowserWidget::EventReceived(const FString JsonResponse)
{
	HandleEvents(JsonResponse);
}

TSharedRef<SWidget> URpmWebBrowserWidget::RebuildWidget()
{
	InitialURL = BuildUrl();

	return Super::RebuildWidget();
}
