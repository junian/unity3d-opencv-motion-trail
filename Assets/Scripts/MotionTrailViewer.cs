using UnityEngine;
using System.Collections;
using System.Runtime.InteropServices;

public class MotionTrailViewer : MonoBehaviour {

	float timeCounter = 0f;
	float fps = 1f / 10f;

	[DllImport ("__Internal")]
	private static extern void MotionTrail(System.IntPtr currentColors, System.IntPtr prevColors, int width, int height);

	WebCamTexture webcamTexture;
	Texture2D texture = null;
	Texture2D prevTexture = null;
	
	// Use this for initialization
	void Start () {
		WebCamDevice[] devices = WebCamTexture.devices;
		if (devices.Length > 0) {
			webcamTexture = new WebCamTexture(devices.Length > 1 ? devices[1].name : devices[0].name ,320, 240, 10);
			webcamTexture.Play();
		}       
	}

	void Update() {
		if (webcamTexture.didUpdateThisFrame) {
			timeCounter += Time.deltaTime;
			if(timeCounter < fps)
				return;
			timeCounter = 0f;
			Color32[] pixels = webcamTexture.GetPixels32 ();
			if(prevTexture == null)
			{
				prevTexture = new Texture2D (webcamTexture.width, webcamTexture.height);
				prevTexture.SetPixels32 (pixels);
				prevTexture.Apply();
				return;
			}

			GCHandle prevPixelHandle = GCHandle.Alloc(prevTexture.GetPixels32(), GCHandleType.Pinned);

			Destroy(prevTexture);
			prevTexture = new Texture2D(webcamTexture.width, webcamTexture.height);
			prevTexture.SetPixels32(webcamTexture.GetPixels32());
			prevTexture.Apply();

			GCHandle currentPixelHandle = GCHandle.Alloc (pixels, GCHandleType.Pinned);
			MotionTrail (currentPixelHandle.AddrOfPinnedObject (), prevPixelHandle.AddrOfPinnedObject(), webcamTexture.width, webcamTexture.height);

			Destroy(texture);
			texture = new Texture2D (webcamTexture.width, webcamTexture.height);
			texture.SetPixels32 (pixels);
			texture.Apply ();

			currentPixelHandle.Free ();
			prevPixelHandle.Free();
			renderer.material.mainTexture = texture;
		}
	}
}
