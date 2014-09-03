// ==UserScript==
// @name           Set wmode to 'direct'
// @namespace      None
// @description    Sets embed's and object's wmode parameter to 'direct' to enable hw acceleration
// @include        *
// @grant          none
// ==/UserScript==

(function ()
{
	nodeInserted();
})();

document.addEventListener("DOMNodeInserted", nodeInserted, false);

function nodeInserted()
{
	for (var objs = document.getElementsByTagName("object"), i = 0, obj; obj = objs[i]; i++)
	{
		if (obj.type == 'application/x-shockwave-flash')
		{
			var skip = false;
			for (var params = obj.getElementsByTagName("param"), j = 0, param; param = params[j]; j++)
			{
				if (param.getAttribute("name") == "wmode")
				{
					param.setAttribute("value", "direct");
					skip = true;
					break;
				}
			}
			if(skip) continue;
			var param = document.createElement("param");
			param.setAttribute("name", "wmode");
			param.setAttribute("value", "direct");
			obj.appendChild(param);
		}
	}

	for (var embeds = document.getElementsByTagName("embed"), i = 0, embed; embed = embeds[i]; i++) {
		if (embed.type != 'application/x-shockwave-flash') continue;
		if ((embed.getAttribute('wmode') && embed.getAttribute('wmode') == 'direct')) continue;
		embed.setAttribute('wmode', 'direct');
		var html = embed.outerHTML;
		embed.insertAdjacentHTML('beforeBegin', embed.outerHTML);
		embed.parentNode.removeChild(embed);
	}
}
